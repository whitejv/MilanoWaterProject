#include <stdio.h>
#include <string.h>
#include <Wire.h>
#include <IPAddress.h>
#include <PubSubClient.h>
//#include <ArduinoJson.h>
#include <cJSON.h>
#include <water.h>

#if defined(ARDUINO_ESP8266_GENERIC) || defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_ESP8266_THING_DEV)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <Adafruit_ADS1X15.h>
//#include <Adafruit_ADT7410.h>
//#include <SparkFun_TCA9534.h>
#elif defined(ARDUINO_FEATHER_ESP32)
#include <WiFi.h>
#include <esp_task_wdt.h>
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#endif

/*
GPIO00 - //Good - Config 3
GPIO01 - stops operation of board
GPIO02 - //Good also lights LED - Temp Sensor
GPIO03 - //Good - Config 2 (Also TX Pin If grounded board won’t load)
GPIO04 - //Good - Disc Input 1
GPIO05 - //Good - Disc Input 2
GPIO06 - doesn’t exist
GPIO12 - //Good - Config 1
GPIO13 - //Good - Flow Sensor
GPIO14 - //reserved SDA
GPIO15 - //reserved SCL
GPIO16 - //Good - Disc 3 (no pull-up)
*/

/* Declare all constants and global variables */

IPAddress prodMqttServerIP(192, 168, 1, 250);
IPAddress devMqttServerIP(192, 168, 1, 249);

int GPIOSens = FALSE;
int tempSens = FALSE;
int extendedSensor = 0;
int sensor = 0;
int secondADC = FALSE;
int InitiateReset = 0;
int ErrState = 0;
int ErrCount = 0;
int WDT_Interval = 0;
const int ERRMAX = 10;
const int WDT_TIMEOUT = 10;
unsigned int masterCounter = 0;
const int discInput1 = DISCINPUT1;
const int discInput2 = DISCINPUT2;
int ioInput = 0;
long currentMillis = 0;
long previousMillis = 0;
long millisecond = 0;
int interval = 2000;
volatile byte pulseCount;
byte pulse1Sec = 0;
unsigned long timerOTA;
char messageName[50];
char messageNameJSON[50];
float temperatureF;

WiFiClient espFlowClient;

PubSubClient P_client(espFlowClient);
PubSubClient D_client(espFlowClient);
PubSubClient client;

const int oneWireBus = TEMPSENSOR;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

//Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads1;     /* 0x48 Use this for the 12-bit version */
//Adafruit_ADS1115 ads2;     /* 0x49 Use this for the 12-bit version */
//Adafruit_ADT7410 tempsensor = Adafruit_ADT7410(); // Create the ADT7410 temperature sensor object

//GPIO Extender Definition  b0-b4 are general use inputs, b5&6 are general outputs, b7 is case fan control
//TCA9534 extendedGPIO1;
//#define NUM_GPIO 8
//bool currentPinMode[NUM_GPIO] = {GPIO_IN, GPIO_IN, GPIO_IN, GPIO_IN, GPIO_IN, GPIO_OUT, GPIO_OUT, GPIO_OUT};
//bool gpioStatus[NUM_GPIO];

/* Forward declaration of functions to solve circular dependencies */
void updateWatchdog();
void updateMasterCounter();
void updateFlowData();
void updateTemperatureData();
void processMqttClient();
void publishFlowData();
void publishJsonData();
void printFlowData();
void printClientState(int state);
void readAnalogInput();
void readDigitalInput();
void readAnalogInputX();
void readDigitalInputX();
void readTempInputX();
void setupOTA();
void setupWiFi();
void connectToMQTTServer();
void checkConnectionAndLogState();

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
    Serial.begin(115200);
    Serial.println("Booting");

  #if defined(ARDUINO_FEATHER_ESP32)
    Serial.printf("Configuring WDT...");
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_add(NULL);
    Serial.printf("Complete\n");
    Wire.begin();
  #elif defined(ARDUINO_ESP8266_GENERIC) || defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_ESP8266_THING_DEV)
    pinMode(LED_BUILTIN, OUTPUT);
    const int configPin1 = CONFIGPIN1;
    const int configPin2 = CONFIGPIN2;
    const int configPin3 = CONFIGPIN3;
    pinMode(configPin1, INPUT_PULLUP);
    pinMode(configPin2, INPUT_PULLUP);
    pinMode(configPin3, INPUT_PULLUP);

    sensors.begin();// Start the DS18B20 sensor
    pinMode(FLOWSENSOR, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FLOWSENSOR), pulseCounter, FALLING);
  #elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    Wire.begin();
  #endif
    delay(3000); //give some time for things to get settled
    // Read the config pins and get configuation data
    //Serial.print(digitalRead(configPin1));
    //Serial.print(digitalRead(configPin2));
    sensor = digitalRead(configPin3) <<2 | digitalRead(configPin2)<<1 | digitalRead(configPin1) ;
    Serial.print(" #");
    Serial.print(sensor);

  /* 
  * Sensors 0-3 are standard sensors
  * Sensors 4-7 are extended sensors with additional data words
  */


    if (sensor >= 4){ 
      extendedSensor = 1;
      Serial.print(" Extended ") ;
    }
    Serial.print(" Sensor ID: ");
    Serial.println(flowSensorConfig[sensor].sensorName);

    
    strcpy(messageName, flowSensorConfig[sensor].messageid);
    strcpy(messageNameJSON, flowSensorConfig[sensor].jsonid);
  
    //if ( extendedSensor == 1 ) {
       //ads1.setGain(GAIN_TWOTHIRDS);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
       //ads2.setGain(GAIN_TWOTHIRDS);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

      // Starting ads1 at 0x48 I²C position
      //if (!ads1.begin(0x48)) {
        //Serial.println("Failed to initialize ADS1.");
        //while (1);
      //}

      // Starting ads2 at 0x49 I²C position
      //if (ads2.begin(0x49) == TRUE) {
         //secondADC = TRUE;
      //}
      //else {
        //Serial.println("No ADC #2 Detected (optional)");
      //}
      // Temp Sensor is 0x4B (if present)
      //if (tempsensor.begin(0x4b) == TRUE ) {
         //tempSens = TRUE;
      //}
      //else {
        //Serial.println("No Temp Sensor Detected (optional)") ;
      //}
      //pinMode(discInput1, INPUT_PULLUP); //in extended mode there is only one onboard GPIO
      // GPIO Sensor is 0x27 (if present)
      //if (extendedGPIO1.begin() == TRUE ) {
         //GPIOSens = TRUE;
         //extendedGPIO1.pinMode(currentPinMode);//Use GPIO_OUT and GPIO_IN instead of OUTPUT and INPUT_PULLUP
      //}
      //else {
        //Serial.println("No GPIO Sensor Detected(optional)") ;
      //}
   // }                                      
    //else {
      pinMode(discInput1, INPUT_PULLUP); //in normal mode the sensor board can support 2 GPIOs
      pinMode(discInput2, INPUT_PULLUP);
   // }
    
    setupWiFi();
    setupOTA();
    connectToMQTTServer();

    if (client.setBufferSize(1024) == FALSE ) {
      Serial.println("Failed to allocate large MQTT send buffer - JSON messages may fail to send.");
    }

}

void loop() {

  ArduinoOTA.handle();
  if (millis() - timerOTA > 500) {
     updateWatchdog();
     updateMasterCounter();
     updateFlowData();
     //updateTemperatureData();
     //readAnalogInput() ;
     //readDigitalInput() ;
     //if ( extendedSensor == 1) {
        //readAnalogInputX();
        //readDigitalInputX();
        //readTempInputX();
     //}
     processMqttClient();
     publishFlowData();
     publishJsonData();
     printFlowData();
     timerOTA = millis();
  }
  checkConnectionAndLogState();

}

/* Implementation of the functions */
void setupWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("");
  Serial.print("WiFi connected -- ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
}

void setupOTA(){
  /*
   * Adding OTA Support
   */
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void connectToMQTTServer(){
  unsigned long connectAttemptStart = millis();
  bool connected = false;
  //Try connecting to the production MQTT server first

  P_client.setServer(prodMqttServerIP, PROD_MQTT_PORT);

  // Connect to the MQTT server
 
  while (!P_client.connected() && millis() - connectAttemptStart < 5000) { // Adjust the timeout as needed
    Serial.print("Connecting to Production MQTT Server: ...");
    connected = P_client.connect(flowSensorConfig[sensor].clientid);
    if (connected) {
      client = P_client; // Assign the connected production client to the global client object
      Serial.println("connected\n");
    } else {
      Serial.print("failed with client state: ");
      printClientState(P_client.state());
      Serial.println() ;
      delay(2000);
    }
  }

  // If connection to the production server failed, try connecting to the development server
  if (!connected) {
    //PubSubClient client(devMqttServerIP, DEV_MQTT_PORT, espWellClient);
    D_client.setServer(devMqttServerIP, DEV_MQTT_PORT);
    while (!D_client.connected()) {
      Serial.print("Connecting to Development MQTT Server...");
      connected = D_client.connect(flowSensorConfig[sensor].clientid);
      if (connected) {
        client = D_client; // Assign the connected development client to the global client object
        Serial.println("connected\n");
      } else {
        Serial.print("failed with client state: ");
        printClientState(D_client.state());
        Serial.println();
        delay(2000);
      }
    }
  }
}

void updateWatchdog() {
  if (WDT_Interval++ > WDT_TIMEOUT) { WDT_Interval = 0; }
  #if defined (ARDUINO_FEATHER_ESP32)
  esp_task_wdt_reset();
  #endif
}

void updateMasterCounter() {
  ++masterCounter;
  if (masterCounter > 28800) {  //Force a reboot every 8 hours
    while (1) {};
  }
  genericSens_.generic.cycle_count = masterCounter;
}

void updateFlowData() {
  currentMillis = millis();
  if (((currentMillis - previousMillis) > interval) && pulseCount > 0 ) {
    pulse1Sec = pulseCount;
    millisecond = millis() - previousMillis ;
    genericSens_.generic.pulse_count = pulse1Sec ;
    genericSens_.generic.milliseconds = millisecond ;
    genericSens_.generic.new_data_flag = 1;
    previousMillis = millis();
  } else {
    genericSens_.generic.new_data_flag = 0 ;
  }
  pulseCount = 0;
}

void updateTemperatureData() {
  sensors.requestTemperatures(); 
  temperatureF = sensors.getTempFByIndex(0);
  genericSens_.generic.temp = (int)temperatureF;
  memcpy(&genericSens_.generic.temp_w1,  &temperatureF, sizeof(temperatureF));
}
/*
void readAnalogInput() {
  const int analogInPin = A0; 
  genericSens_.generic.adc_sensor = analogRead(analogInPin);
  //Serial.println(flow_data_payload[3]);
}

void readAnalogInputX() {
  //Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");

  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV


  int16_t adc0, adc1, adc2, adc3, adc4, adc5, adc6, adc7;
  float volts0, volts1, volts2, volts3, volts4, volts5, volts6, volts7;

  adc0 = ads1.readADC_SingleEnded(0);
  adc1 = ads1.readADC_SingleEnded(1);
  adc2 = ads1.readADC_SingleEnded(2);
  adc3 = ads1.readADC_SingleEnded(3);

  genericSens_.generic.adc_x1 = adc0;
  genericSens_.generic.adc_x2 = adc1;
  genericSens_.generic.adc_x3 = adc2;
  genericSens_.generic.adc_x4 = adc3;
  
  if ( secondADC == TRUE ) {
      adc4 = ads2.readADC_SingleEnded(0);
      adc5 = ads2.readADC_SingleEnded(1);
      adc6 = ads2.readADC_SingleEnded(2);
      adc7 = ads2.readADC_SingleEnded(3);

      genericSens_.generic.adc_x5 = adc4;
      genericSens_.generic.adc_x6 = adc5;
      genericSens_.generic.adc_x7 = adc6;
      genericSens_.generic.adc_x8 = adc7;
  }
  /*
  volts0 = ads1.computeVolts(adc0);
  volts1 = ads1.computeVolts(adc1);
  volts2 = ads1.computeVolts(adc2);
  volts3 = ads1.computeVolts(adc3);
  volts4 = ads2.computeVolts(adc4);
  volts5 = ads2.computeVolts(adc5);
  volts6 = ads2.computeVolts(adc6);
  volts7 = ads2.computeVolts(adc7);

  Serial.println("-----------------------------------------------------------");
  Serial.print("AIN0: "); Serial.print(adc0); Serial.print("  "); Serial.print(volts0); Serial.println("V");
  Serial.print("AIN1: "); Serial.print(adc1); Serial.print("  "); Serial.print(volts1); Serial.println("V");
  Serial.print("AIN2: "); Serial.print(adc2); Serial.print("  "); Serial.print(volts2); Serial.println("V");
  Serial.print("AIN3: "); Serial.print(adc3); Serial.print("  "); Serial.print(volts3); Serial.println("V");
  Serial.println("+++++++++++++++++++");
  Serial.print("AIN4: "); Serial.print(adc4); Serial.print("  "); Serial.print(volts4); Serial.println("V");
  Serial.print("AIN5: "); Serial.print(adc5); Serial.print("  "); Serial.print(volts5); Serial.println("V");
  Serial.print("AIN6: "); Serial.print(adc6); Serial.print("  "); Serial.print(volts6); Serial.println("V");
  Serial.print("AIN7: "); Serial.print(adc7); Serial.print("  "); Serial.print(volts7); Serial.println("V");
  
}*/
/*
void readTempInputX() {
  if (tempSens == TRUE) {
    float c = tempsensor.readTempC();
    float f = c * 9.0 / 5.0 + 32;
    
    //Serial.print("Temp: "); Serial.print(c); Serial.print("*C\t");
    //Serial.print(f); Serial.println("*F");

    memcpy(&genericSens_.generic.GPIO_x2, &f, sizeof(float));
  }
}
void readDigitalInput() {

  // Read the config pins and get configuation data
  //Serial.print(digitalRead(discInput1));
  //Serial.print(digitalRead(discInput2));
  ioInput = digitalRead(discInput2)<<1 | digitalRead(discInput1) ;
  genericSens_.generic.gpio_sensor = ioInput ;
  //Serial.print("IO Input: ");
  //Serial.println(ioInput);
}
void readDigitalInputX() {
//There are two ways to read from a port, either by returning the full register value as a uint8_t, or passing in an array of 8 boolean's to be modified by the function with the statuses of each pin
  uint8_t portValue = extendedGPIO1.digitalReadPort(gpioStatus);
  
  Serial.print("uint8_t: ");
  Serial.println(portValue, BIN);
  Serial.print("Bool array: ");
  for (uint8_t arrayPosition = 0; arrayPosition < NUM_GPIO; arrayPosition++) {
    Serial.print(arrayPosition);
    Serial.print(": ");
    switch (gpioStatus[arrayPosition])
    {
      case true:
        Serial.print("HI ");
        break;
      case false:
        Serial.print("LO ");
        break;
    }
  }
  Serial.println("\n");
}
*/
void processMqttClient() {
  client.loop();
}

void publishFlowData() {
  client.publish(flowSensorConfig[sensor].messageid, (byte *)genericSens_.data_payload, flowSensorConfig[sensor].messagelen*4);
}

void publishJsonData() {

    int i;
    
    // Create a new cJSON object
    cJSON *jsonDoc = cJSON_CreateObject();

    //Serial.printf("message length   %d", flowSensorConfig[sensor].messagelen);
    for (i = 0; i < flowSensorConfig[sensor].messagelen; i++) {
        // Add data to the cJSON object
        cJSON_AddNumberToObject(jsonDoc, genericsens_ClientData_var_name[i], genericSens_.data_payload[i]);
    }

    // Serialize the cJSON object to a string
    char *jsonBuffer = cJSON_Print(jsonDoc);
    if (jsonBuffer != NULL) {
        size_t n = strlen(jsonBuffer);
        //Serial.printf(flowSensorConfig[sensor].jsonid);
        //Serial.printf("   %d", n);
        //Serial.printf("\n");
        //Serial.printf(jsonBuffer);
        //Serial.printf("\n");
        
        // Publish the JSON data
        if (client.publish(flowSensorConfig[sensor].jsonid, jsonBuffer, n) == FALSE) {
          Serial.printf("JSON Message Failed to Publish");
          Serial.printf("\n");
        }
        // Free the serialized data buffer
        free(jsonBuffer);
    }

    // Delete the cJSON object
    cJSON_Delete(jsonDoc);
}
void printFlowData() {
  Serial.printf(messageName);
  //Serial.printf("message length   %d",flowSensorConfig[sensor].messagelen); 
  for (int i = 0; i<=flowSensorConfig[sensor].messagelen-1; ++i) {
    Serial.printf(" %x", genericSens_.data_payload[i]);
  }
  Serial.printf("\n");
}
void printClientState(int state) {
  switch (state) {
    case -4:
      Serial.println("MQTT_CONNECTION_TIMEOUT");
      break;
    case -3:
      Serial.println("MQTT_CONNECTION_LOST");
      break;
    case -2:
      Serial.println("MQTT_CONNECT_FAILED");
      break;
    case -1:
      Serial.println("MQTT_DISCONNECTED");
      break;
    case  0:
      Serial.println("MQTT_CONNECTED");
      break;
    case  1:
      Serial.println("MQTT_CONNECT_BAD_PROTOCOL");
      break;
    case  2:
      Serial.println("MQTT_CONNECT_BAD_CLIENT_ID");
      break;
    case  3:
      Serial.println("MQTT_CONNECT_UNAVAILABLE");
      break;
    case  4:
      Serial.println("MQTT_CONNECT_BAD_CREDENTIALS");
      break;
    case  5:
      Serial.println("MQTT_CONNECT_UNAUTHORIZED");
      break;
  }
}
void checkConnectionAndLogState(){
    if (client.connected() == FALSE) {
    ErrState = client.state() ;
    ++ErrCount;
    Serial.printf(messageName);
    Serial.print("-- Disconnected from MQTT:");
    Serial.print("Error Count:  ");
    Serial.print(ErrCount);
    Serial.print("Error Code:  ");
    Serial.println(ErrState);
  }

  if ( ErrCount > ERRMAX ) {
    //Initiate Reset
    Serial.println("Initiate board reset!!") ;
    while(1);
  }
}