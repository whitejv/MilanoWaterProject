#include <stdio.h>
#include <string.h>
#include <Wire.h>
#include <IPAddress.h>
#include <PubSubClient.h>

/* NCD ESP8266 Board - Select GENERIC ESP8266 MODULE
 * in Arduino Board Manager 
 */
 #if defined (ARDUINO_ESP8266_GENERIC)
   #include <ESP8266WiFi.h>
   #include <OneWire.h>
   #include <DallasTemperature.h>
#endif
/* WEMOS D1R1 ESP8266 Board - Amazon 3/10/2023
 * in Arduino Board Manager 
 */
#if defined (ARDUINO_ESP8266_WEMOS_D1R1)
   #include <ESP8266WiFi.h>
   #include <OneWire.h>
   #include <DallasTemperature.h>
#endif
/* NCD ESP32 Board - Select Adafruit ESP32 Feather
 * in Arduino Board Manager 
 */
#if defined (ARDUINO_FEATHER_ESP32)
   #include <WiFi.h>
   #include <esp_task_wdt.h>
#endif
/* RPI PICO W - Select ARDUINO_RASPBERRY_PI_PICO_W
 * in Arduino Board Manager 
 */
#if defined (ARDUINO_RASPBERRY_PI_PICO_W)
   #include <WiFi.h>
   #include "pico/stdlib.h"
   #include "hardware/i2c.h"
   #include "pico/binary_info.h"
#endif

#define WDT_TIMEOUT 10  //10 seconds Watch Dog Timer (WDT)

char ssid[] = "ATT9LCV8fL_2.4";    //local wifi network SSID
char password[] = "6jhz7ai7pqy5";  //local network password

IPAddress MQTT_BrokerIP(192, 168, 1, 249);
const char *mqttServer = "raspberrypi.local";
const int mqttPort = 1883;

//const char *mqttServer = "soldier.cloudmqtt.com";
//const int mqttPort = 15599;
//const char *mqttUser = "zerlcpdf";
//const char *mqttPassword = "OyHBShF_g9ya";

WiFiClient espFlowClient;

PubSubClient client(MQTT_BrokerIP, mqttPort, espFlowClient);

int WDT_Interval = 0;
unsigned int masterCounter = 0;

/*
 * Data Block Interface Control
 */

/*
* payload 0	Pulses Counted in Time Window
* payload 1	Number of milliseconds in Time Window
* payload 2	Flag 1=new data 0=stale data
* payload 3	Pressure Sensor Analog Value
* payload 4	 unused
* payload 5	 unused
* payload 6	 unused
* payload 7	 unused
* payload 8	 unused
* payload 9	 unused
* payload 10	 unused
* payload 11	 unused
* payload 12	 Cycle Counter 16bit Int
* payload 13	 unused
* payload 14	 unused
* payload 15	 unused
* payload 16	 unused
* payload 17	Temperature in F
* payload 18	 unused
* payload 19	 unused
* payload 20	 FW Version 4 Hex 
*/	
/*	
* payload 21	 Last payload is Control Word From User
*/ 	


int raw_flow_data[22] =   { 0, 0, 0, 0, 0,
                                           0, 0, 0, 0, 0,
                                           0, 0, 0, 0, 0,
                                           0, 0, 0, 0, 0, 0,
                                           0 };

#define FLOWSENSOR  13
 
long currentMillis = 0;
long previousMillis = 0;
long millisecond = 0;
int interval = 2000;
volatile byte pulseCount;
byte pulse1Sec = 0;

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}
// GPIO where the DS18B20 is connected to
const int oneWireBus = 2;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

void setup() {

  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

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
    //if (client.connect("ESP8266FlowClient", mqttUser, mqttPassword))
    if (client.connect("ESP8266FlowClient")) {
      Serial.printf("connected\n");
    } else {
      Serial.printf("failed with ");
      Serial.printf("client state %d\n", client.state());
      delay(2000);
    }
  }
  client.subscribe("ESP Control");

#if defined (ARDUINO_FEATHER_ESP32)
  Serial.printf("Configuring WDT...");
  esp_task_wdt_init(WDT_TIMEOUT, true);  //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);                //add current thread to WDT watch
  Serial.printf("Complete\n");
  Wire.begin();
#endif
#if defined (ARDUINO_ESP8266_GENERIC)
  Wire.begin(12,14);
  pinMode(FLOWSENSOR, INPUT_PULLUP);  
  sensors.begin(); // Start the DS18B20 sensor
  attachInterrupt(digitalPinToInterrupt(FLOWSENSOR), pulseCounter, FALLING);
#endif
#if defined (ARDUINO_RASPBERRY_PI_PICO_W)
  i2c_init(i2c_default, 100 * 1000);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  //gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  //gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
  Wire.begin();
#endif
pulseCount = 0;
previousMillis = 0;
}
void loop() {
  int i;
  int decimal;


/* If I2C Errors Exceed 100 in 10 seconds then force a reboot to hopefully clear up */
/*
  if (((I2CPanicCount - last_I2CPanicCount) > 25) && (WDT_Interval >= WDT_TIMEOUT)) {
    Serial.printf("%d %d %d\n", I2CPanicCount, last_I2CPanicCount, WDT_Interval);
    while (1) {}  //force a reboot
  }
*/
  if (WDT_Interval++ > WDT_TIMEOUT) { WDT_Interval = 0; }

  #if defined (ARDUINO_FEATHER_ESP32)
  esp_task_wdt_reset();
  #endif

  //I2CComm();

  ++masterCounter;

  if (masterCounter > 28800) {  //Force a reboot every 8 hours
    while (1) {};
  }
  currentMillis = millis();
  if (((currentMillis - previousMillis) > interval) && pulseCount > 0 ) {
    pulse1Sec = pulseCount;
    millisecond = millis() - previousMillis ;
    raw_flow_data[0] = pulse1Sec ;
    raw_flow_data[1] = millisecond ;
    raw_flow_data[2] = 1;
    previousMillis = millis();
  } else {
    raw_flow_data[2] = 0 ;
  }
  pulseCount =0 ;


  sensors.requestTemperatures(); 
  float temperatureF = sensors.getTempFByIndex(0);
  //printf("%f \n", temperatureF) ;
  raw_flow_data[17] = *((int *)&temperatureF);
// store first 2 bytes of float into i[0]
// be careful here!!! endianness matters
// for little endian, use
// i[0] = *(((int16_t *)&f) + 1);

//raw_flow_data[18] = *(((int16_t *)&temperatureF) + 1);
// store last 2 bytes of float into i[1]
// be careful here!!! endianness matters
// for little endian, use
// i[1] = *(((int16_t *)&f) + 0);

  
  raw_flow_data[12] = masterCounter;
 // raw_sensor_data[16] = I2CPanicCount;
  raw_flow_data[3] = analogRead(A0);  // This reads the analog in value
  
  client.loop();

  client.publish("Flow ESP", (byte *)raw_flow_data, 42);

  Serial.printf("FLow Data: ");
  for (i = 0; i <= 16; ++i) {
    Serial.printf("%x ", raw_flow_data[i]);
  }
  for (i = 17; i <= 17; ++i) {
    Serial.printf("%f ", *((float *)&raw_flow_data[i]));
  }
  for (i = 19; i <= 20; ++i) {
    Serial.printf("%x ", raw_flow_data[i]);
  }
  Serial.printf("\n");

  delay(500);
}