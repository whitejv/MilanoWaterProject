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

int InitiateReset = 0;
int ErrState = 0 ;
int ErrCount = 0 ;
#define ERRMAX 10 
#define WDT_TIMEOUT 10  //10 seconds Watch Dog Timer (WDT)

const char ssid[] = "ATT9LCV8fL_2.4";
const char password[] = "6jhz7ai7pqy5";

WiFiClient espFlowClient;

/* Define the IPs for Production and Development MQTT Servers */
IPAddress prodMqttServerIP(192, 168, 1, 250);
IPAddress devMqttServerIP(192, 168, 1, 249) ;
PubSubClient P_client(espFlowClient);
PubSubClient D_client(espFlowClient);


PubSubClient client; // Declare the client object globally

int WDT_Interval = 0;
unsigned int masterCounter = 0;

#define FLOWSENSOR 13

unsigned long timerOTA ;

long currentMillis = 0;
long previousMillis = 0;
long millisecond = 0;
int interval = 2000;
volatile byte pulseCount;
byte pulse1Sec = 0;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

const int oneWireBus = 2;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);

  Serial.println("Booting");
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
  
unsigned long connectAttemptStart = millis();
bool connected = false;

//Try connecting to the production MQTT server first

P_client.setServer(prodMqttServerIP, PROD_MQTT_PORT);

// Connect to the MQTT server

while (!P_client.connected() && millis() - connectAttemptStart < 5000) { // Adjust the timeout as needed
  Serial.print("Connecting to Production MQTT Server: ...");
  connected = P_client.connect(FLOW_CLIENTID);
  if (connected) {
    client = P_client; // Assign the connected production client to the global client object
    Serial.println("connected\n");
    //digitalWrite(LEDB, HIGH);
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
    connected = D_client.connect(FLOW_CLIENTID);
    if (connected) {
      client = D_client; // Assign the connected development client to the global client object
      Serial.println("connected\n");
      //digitalWrite(LEDB, HIGH);
    } else {
      Serial.print("failed with client state: ");
      printClientState(D_client.state());
      Serial.println();
      delay(2000);
    }
  }
}

  //client.subscribe("ESP Control");

#if defined(ARDUINO_FEATHER_ESP32)
  Serial.printf("Configuring WDT...");
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.printf("Complete\n");
  Wire.begin();
#elif defined(ARDUINO_ESP8266_GENERIC) || defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ARDUINO_ESP8266_THING_DEV)
  //Wire.begin(12, 14); //only if you are using I2C
  pinMode(FLOWSENSOR, INPUT_PULLUP);
  sensors.begin();// Start the DS18B20 sensor
  attachInterrupt(digitalPinToInterrupt(FLOWSENSOR), pulseCounter, FALLING);
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
  i2c_init(i2c_default, 100 * 1000);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  Wire.begin();
#endif

  pulseCount = 0;
  previousMillis = 0;

  timerOTA = millis();
}
void loop() {
  ArduinoOTA.handle() ;
  if (millis() - timerOTA > 500) {
     updateWatchdog();
     updateMasterCounter();
     updateFlowData();
     updateTemperatureData();
     readAnalogInput();
     readDigitalInput();
     processMqttClient();
     publishFlowData();
     publishJsonData();
     printFlowData();
     //delay(500);
     timerOTA = millis() ;
  }
  /*
 * Check Connection and Log State then Determine if a Reset is necessary
 */

  if (client.connected() == FALSE) {
    ErrState = client.state() ;
    ++ErrCount;
    Serial.print("Flow Monitor Disconnected from MQTT:");
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
  flow_data_payload[12] = masterCounter;
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
 float temperatureF ;
void updateTemperatureData() {
  sensors.requestTemperatures(); 
  temperatureF = sensors.getTempFByIndex(0);
  flow_data_payload[17] = *((int *)&temperatureF);
}

void readAnalogInput() {
  flow_data_payload[3] = analogRead(A0);
}

void readDigitalInput() {

}

void processMqttClient() {
  client.loop();
}

void publishFlowData() {
  client.publish(FLOW_CLIENT, (byte *)flow_data_payload, FLOW_LEN*4);
}

void publishJsonData() {
  const size_t capacity = JSON_OBJECT_SIZE(10);
  StaticJsonDocument<capacity> jsonDoc;

  jsonDoc["Pulses Counted"] = flow_data_payload[0];
  jsonDoc["Elapsed MilliSec"] = flow_data_payload[1];
  jsonDoc["New Data Flag"] = flow_data_payload[2];
  jsonDoc["Raw Hydrostatic Press"] = flow_data_payload[3];
  jsonDoc["Counter"] = flow_data_payload[12];
  jsonDoc["Air Temp"] = temperatureF;
  

  char jsonBuffer[256];
  size_t n = serializeJson(jsonDoc, jsonBuffer);

  client.publish("FLOW JSON", jsonBuffer, n);
}
void printFlowData() {
  Serial.printf("Irrigation FLow Data: ");
  for (int i = 0; i <= 16; ++i) {
    Serial.printf("%x ", flow_data_payload[i]);
  }
  Serial.printf("%f ", *((float *)&flow_data_payload[17]));
  for (int i = 19; i <= 20; ++i) {
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