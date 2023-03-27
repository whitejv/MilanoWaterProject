#include <stdio.h>
#include <string.h>
#include <Wire.h>
#include <IPAddress.h>
#include <PubSubClient.h>

#if defined(ARDUINO_ESP8266_GENERIC) || defined(ARDUINO_ESP8266_WEMOS_D1MINI)
#include <ESP8266WiFi.h>
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

#define WDT_TIMEOUT 10  //10 seconds Watch Dog Timer (WDT)

const char ssid[] = "ATT9LCV8fL_2.4";
const char password[] = "6jhz7ai7pqy5";

IPAddress MQTT_BrokerIP(192, 168, 1, 249);
const char *mqttServer = "raspberrypi.local";
const int mqttPort = 1883;

WiFiClient espFlowClient;
PubSubClient client(MQTT_BrokerIP, mqttPort, espFlowClient);

int WDT_Interval = 0;
unsigned int masterCounter = 0;

int raw_tank_data[22] = { 0 };

#define FLOWSENSOR 13

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
  delay(10);
  
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected -- ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(MQTT_BrokerIP, mqttPort);

  while (!client.connected()) {
    Serial.printf("Connecting to MQTT.....");
    if (client.connect("ESP8266TankClient")) {
      Serial.printf("connected\n");
    } else {
      Serial.printf("failed with ");
      Serial.printf("client state %d\n", client.state());
      delay(2000);
    }
  }

  client.subscribe("ESP Control");

#if defined(ARDUINO_FEATHER_ESP32)
  Serial.printf("Configuring WDT...");
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.printf("Complete\n");
  Wire.begin();
#elif defined(ARDUINO_ESP8266_GENERIC) || defined(ARDUINO_ESP8266_WEMOS_D1MINI)
  Wire.begin(12, 14);
  pinMode(FLOWSENSOR, INPUT_PULLUP);
  sensors.begin();
  attachInterrupt(digitalPinToInterrupt(FLOWSENSOR), pulseCounter, FALLING);
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
  i2c_init(i2c_default, 100 * 1000);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  Wire.begin();
#endif

  pulseCount = 0;
  previousMillis = 0;
}
void loop() {
  updateWatchdog();
  updateMasterCounter();
  updateFlowData();
  updateTemperatureData();
  readAnalogInput();
  processMqttClient();
  publishFlowData();
  printFlowData();
  delay(500);
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
}

void updateFlowData() {
  currentMillis = millis();
  if (((currentMillis - previousMillis) > interval) && pulseCount > 0 ) {
    pulse1Sec = pulseCount;
    millisecond = millis() - previousMillis ;
    raw_tank_data[0] = pulse1Sec ;
    raw_tank_data[1] = millisecond ;
    raw_tank_data[2] = 1;
    previousMillis = millis();
  } else {
    raw_tank_data[2] = 0 ;
  }
  pulseCount = 0;
}

void updateTemperatureData() {
  sensors.requestTemperatures(); 
  float temperatureF = sensors.getTempFByIndex(0);
  raw_tank_data[17] = *((int *)&temperatureF);
}

void readAnalogInput() {
  raw_tank_data[3] = analogRead(A0);
}

void processMqttClient() {
  client.loop();
}

void publishFlowData() {
  client.publish("Flow ESP", (byte *)raw_tank_data, 42);
}

void printFlowData() {
  Serial.printf("FLow Data: ");
  for (int i = 0; i <= 16; ++i) {
    Serial.printf("%x ", raw_tank_data[i]);
  }
  Serial.printf("%f ", *((float *)&raw_tank_data[17]));
  for (int i = 19; i <= 20; ++i) {
    Serial.printf("%x ", raw_tank_data[i]);
  }
  Serial.printf("\n");
}