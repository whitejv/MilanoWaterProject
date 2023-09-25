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
 * For Reference - D# vs GPIO# Reference Table
*static const uint8_t D0   = 3;
*static const uint8_t D1   = 1;
*static const uint8_t D2   = 16;
*static const uint8_t D3   = 5;
*static const uint8_t D4   = 4;
*static const uint8_t D5   = 14;
*static const uint8_t D6   = 12;
*static const uint8_t D7   = 13;
*static const uint8_t D8   = 0;
*static const uint8_t D9   = 2;
*static const uint8_t D10  = 15;
*static const uint8_t D11  = 13;
*static const uint8_t D12  = 12;
*static const uint8_t D13  = 14;
*static const uint8_t D14  = 4;
*static const uint8_t D15  = 5;
*/

/* Declare all constants and global variables */

IPAddress prodMqttServerIP(192, 168, 1, 250);
IPAddress devMqttServerIP(192, 168, 1, 249);
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
  sensor = digitalRead(configPin2)<<1 | digitalRead(configPin1) ;
  Serial.print(sensor);
  //sensor = 3;
  Serial.print("  Sensor ID: ");
  Serial.println(flowSensorConfig[sensor].sensorName);

  
  strcpy(messageName, flowSensorConfig[sensor].sensorName);
  strcat(messageName, " Flow Data: ");
  strcpy(messageNameJSON, "JSON - ");
  strcat(messageNameJSON, flowSensorConfig[sensor].messageid);

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
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED on
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED off)
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
 digitalWrite(LED_BUILTIN, LOW);   // turn the LED on
  while (!P_client.connected() && millis() - connectAttemptStart < 5000) { // Adjust the timeout as needed
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED off
    Serial.print("Connecting to Production MQTT Server: ...");
    connected = P_client.connect(flowSensorConfig[sensor].clientid);
    if (connected) {
      client = P_client; // Assign the connected production client to the global client object
      Serial.println("connected\n");
      digitalWrite(LED_BUILTIN, LOW);   // turn the LED on
    } else {
      Serial.print("failed with client state: ");
      printClientState(P_client.state());
      Serial.println() ;
      delay(2000);
    }
  }

  // If connection to the production server failed, try connecting to the development server
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED on
  if (!connected) {
    //PubSubClient client(devMqttServerIP, DEV_MQTT_PORT, espWellClient);
    D_client.setServer(devMqttServerIP, DEV_MQTT_PORT);
    while (!D_client.connected()) {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED off
      Serial.print("Connecting to Development MQTT Server...");
      connected = D_client.connect(flowSensorConfig[sensor].clientid);
      if (connected) {
        client = D_client; // Assign the connected development client to the global client object
        Serial.println("connected\n");
       digitalWrite(LED_BUILTIN, LOW);   // turn the LED on digitalWrite(5, HIGH);
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
  flow_data_payload[8] = masterCounter;
}

void updateFlowData() {
  currentMillis = millis();
  if (((currentMillis - previousMillis) > interval) && pulseCount > 0 ) {
    pulse1Sec = pulseCount;
    millisecond = millis() - previousMillis ;
    flow_data_payload[0] = pulse1Sec ;
    flow_data_payload[1] = millisecond ;
    flow_data_payload[2] = 1;
    previousMillis = millis();
  } else {
    flow_data_payload[2] = 0 ;
  }
  pulseCount = 0;
}

void updateTemperatureData() {
  sensors.requestTemperatures(); 
  temperatureF = sensors.getTempFByIndex(0);
  flow_data_payload[5] = (int)temperatureF;
  memcpy(&flow_data_payload[6],  &temperatureF, sizeof(temperatureF));
}

void readAnalogInput() {
  const int analogInPin = A0; 
  flow_data_payload[3] = analogRead(analogInPin);
  //Serial.println(flow_data_payload[3]);
}

void readDigitalInput() {

  // Read the config pins and get configuation data
  //Serial.print(digitalRead(discInput1));
  //Serial.print(digitalRead(discInput2));
  ioInput = digitalRead(discInput2)<<1 | digitalRead(discInput1) ;
  flow_data_payload[4] = ioInput ;
  //Serial.print("IO Input: ");
  //Serial.println(ioInput);
}

void processMqttClient() {
  client.loop();
}

void publishFlowData() {
  client.publish(flowSensorConfig[sensor].messageid, (byte *)flow_data_payload, flowSensorConfig[sensor].messagelen*4);
}

void publishJsonData() {
  const size_t capacity = JSON_OBJECT_SIZE(10);
  StaticJsonDocument<capacity> jsonDoc;

  jsonDoc["Pulses Counted"]    = flow_data_payload[0];
  jsonDoc["Elapsed MilliSec"]  = flow_data_payload[1];
  jsonDoc["New Data Flag"]     = flow_data_payload[2];
  jsonDoc["ADC_Sensor"]         = flow_data_payload[3];
  jsonDoc["GPIO_Sensor"]        = flow_data_payload[4];
  jsonDoc["Temperature F(int)"]= flow_data_payload[5];
  jsonDoc["Temperature F"]     = temperatureF;
  jsonDoc["Counter"]           = flow_data_payload[8];
  jsonDoc["FW Version"]        = flow_data_payload[9];
  

  char jsonBuffer[256];
  size_t n = serializeJson(jsonDoc, jsonBuffer);

  client.publish(messageNameJSON, jsonBuffer, n);
}
void printFlowData() {
  Serial.printf(messageName);
  for (int i = 0; i <= 5; ++i) {
    Serial.printf("%x ", flow_data_payload[i]);
  }
  Serial.printf("%f ", *((float *)&flow_data_payload[6]));
  for (int i = 7; i <= 8; ++i) {
    Serial.printf("%x ", flow_data_payload[i]);
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