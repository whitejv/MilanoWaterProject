#include <stdio.h>
#include <string.h>
#include <Wire.h>
#include <IPAddress.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <water.h>

#if defined(ARDUINO_ESP8266_GENERIC) || defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_ESP8266_THING_DEV)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_ADS1X15.h>
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

int extendedSeneor = 0;
int sensor = 0;
int InitiateReset = 0;
int ErrState = 0;
int ErrCount = 0;
int WDT_Interval = 0;
const int ERRMAX = 10;
const int WDT_TIMEOUT = 10;
unsigned int masterCounter = 0;
const int discInput1 = DISCINPUT1;
const int discInput2 = DISCINPUT2;
const int discInput3 = DISCINPUT3;
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
Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */

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
  pinMode(configPin1, INPUT_PULLUP);
  pinMode(configPin2, INPUT_PULLUP);
  pinMode(discInput1, INPUT_PULLUP);
  pinMode(discInput2, INPUT_PULLUP);
  pinMode(discInput3, INPUT_PULLUP);
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
  sensor = digitalRead(configPin3 <<2 | configPin2)<<1 | digitalRead(configPin1) ;
  Serial.print(sensor);

/* 
 * Sensors 0-3 are standard sensors
 * Sensors 4-7 are extended sensors with additional data words
 */


  if (sensor > 3){ extendedSensor == 1} ;
  Serial.print("  Sensor ID: ");
  Serial.println(flowSensorConfig[sensor].sensorName);

  
  strcpy(messageName, flowSensorConfig[sensor].messageid);
  strcpy(messageNameJSON, flowSensorConfig[sensor].jsonid);
 
  if ( extendedSensor == 1 ) {
    if (!ads.begin()) {
       Serial.println("Failed to initialize ADS.");
       while (1);
    }
  }
  setupWiFi();
  setupOTA();
  connectToMQTTServer();

}

void loop() {

  ArduinoOTA.handle();
  if (millis() - timerOTA > 500) {
     updateWatchdog();
     updateMasterCounter();
     updateFlowData();
     updateTemperatureData();
     readAnalogInput() ;
     readDigitalInput() ;
     if ( extendedSensor == 1) {
        readAnalogInputX();
        readDigitalInputX();
     }
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


  int16_t adc0, adc1, adc2, adc3;
  float volts0, volts1, volts2, volts3;

  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);
  //genericSens_.generic.adc_sensor = adc0;
  volts0 = ads.computeVolts(adc0);
  volts1 = ads.computeVolts(adc1);
  volts2 = ads.computeVolts(adc2);
  volts3 = ads.computeVolts(adc3);

  Serial.println("-----------------------------------------------------------");
  Serial.print("AIN0: "); Serial.print(adc0); Serial.print("  "); Serial.print(volts0); Serial.println("V");
  Serial.print("AIN1: "); Serial.print(adc1); Serial.print("  "); Serial.print(volts1); Serial.println("V");
  Serial.print("AIN2: "); Serial.print(adc2); Serial.print("  "); Serial.print(volts2); Serial.println("V");
  Serial.print("AIN3: "); Serial.print(adc3); Serial.print("  "); Serial.print(volts3); Serial.println("V");
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

void processMqttClient() {
  client.loop();
}

void publishFlowData() {
  client.publish(flowSensorConfig[sensor].messageid, (byte *)genericSens_.data_payload, flowSensorConfig[sensor].messagelen*4);
}

void publishJsonData() {
  int i;
  const size_t capacity = JSON_OBJECT_SIZE(20);
  StaticJsonDocument<capacity> jsonDoc;

  for (i=0; i<=GENERICSENS_LEN-1; i++) {
    jsonDoc[genericsens_ClientData_var_name [i]] = genericSens_.data_payload[i];
  }

  char jsonBuffer[2048];
  size_t n = serializeJson(jsonDoc, jsonBuffer);
  //Serial.printf(flowSensorConfig[sensor].jsonid);
  //Serial.printf("   %d",n);
  //Serial.printf("\n");
  //Serial.printf(jsonBuffer);
  client.publish(flowSensorConfig[sensor].jsonid, jsonBuffer, n);
}
void printFlowData() {
  Serial.printf(messageName);
  for (int i = 0; i <= 5; ++i) {
    Serial.printf(" %x", genericSens_.data_payload[i]);
  }
  Serial.printf("%f ", *((float *)&genericSens_.data_payload[6]));
  for (int i = 7; i <= 8; ++i) {
    Serial.printf("%x ", genericSens_.data_payload[i]);
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