#include <stdio.h>
#include <string.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <IPAddress.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Arduino_LSM6DSOX.h>
#include "hardware/watchdog.h" // Include the watchdog header
#include <water.h>
// ---------------------------------------------------------------------------
// Example NewPing library sketch that does a ping about 20 times per second.
// ---------------------------------------------------------------------------
#include <NewPing.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTTYPE    DHT11     // DHT 11
#define DHTPIN 8
DHT_Unified dht(DHTPIN, DHTTYPE);

#define TRIGGER_PIN  12  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     11  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

char ssid[] = "ATT9LCV8fL_2.4";   // local wifi network SSID
char password[] = "6jhz7ai7pqy5"; // local network password

int InitiateReset = 0;
int ErrState = 0 ;
int ErrCount = 0 ;

// Watchdog timeout period in milliseconds
const int watchdogTimeoutMs = 3000; // 6 seconds, adjust as needed

#define ERRMAX 10 
#define firmwareVer 0x8004

WiFiClient esptankgalClient;

/* Define the IPs for Production and Development MQTT Servers */
IPAddress prodMqttServerIP(192, 168, 1, 250);
IPAddress devMqttServerIP(192, 168, 1, 249) ;
PubSubClient P_client(esptankgalClient);
PubSubClient D_client(esptankgalClient);


PubSubClient client; // Declare the client object globally
unsigned int masterCounter = 0;
float temp, hum;
double speedOfSound, distance, duration;
static int lastMsec = 0;
/*
 * Data Block Interface Control
 */


void setup()
{
  digitalWrite(LEDR, HIGH);
  //Initialize Serial
  Serial.begin(115200);

 
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.print("Communication with WiFi module failed!");
    digitalWrite(LEDR, HIGH);
    while (true);
  }

  // We start by connecting to a WiFi network

  Serial.print("Connecting to - ");
  Serial.print(ssid);
 
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(500);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected -- ");
  Serial.print("Local IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LEDG, HIGH);
  
unsigned long connectAttemptStart = millis();
bool connected = false;

//Try connecting to the production MQTT server first

P_client.setServer(prodMqttServerIP, PROD_MQTT_PORT);

// Connect to the MQTT server

while (!P_client.connected() && millis() - connectAttemptStart < 5000) { // Adjust the timeout as needed
  Serial.print("Connecting to Production MQTT Server: ...");
  connected = P_client.connect(TANKGAL_CLIENTID);
  if (connected) {
    client = P_client; // Assign the connected production client to the global client object
    Serial.println("connected\n");
    digitalWrite(LEDB, HIGH);
  } else {
    Serial.print("failed with client state: ");
    printClientState(P_client.state());
    Serial.println() ;
    delay(2000);
  }
}

// If connection to the production server failed, try connecting to the development server

if (!connected) {
  //PubSubClient client(devMqttServerIP, DEV_MQTT_PORT, esptankgalClient);
  D_client.setServer(devMqttServerIP, DEV_MQTT_PORT);
  while (!D_client.connected()) {
    Serial.print("Connecting to Development MQTT Server...");
    connected = D_client.connect(TANKGAL_CLIENTID);
    if (connected) {
      client = D_client; // Assign the connected development client to the global client object
      Serial.println("connected\n");
      digitalWrite(LEDB, HIGH);
    } else {
      Serial.print("failed with client state: ");
      printClientState(D_client.state());
      Serial.println();
      delay(2000);
    }
  }
}

client.setKeepAlive(45);

  //client.subscribe("ESP Control");
  
  // Enable the watchdog timer
  watchdog_enable(watchdogTimeoutMs, 1);
  Serial.println("Watchdog Timer Enabled\n");

  if (!IMU.begin()) {  
    //Serial.println("Failed to initialize IMU!");
    while (1);
  }
  /*
   * Setup Sensors
   */

  dht.begin();
  sensor_t sensor;
  lastMsec = millis() ;
  analogReadResolution(12);   
}

void loop()
{
int i;
int decimal;
int temperature_deg = 0;
int rawdistance; 
sensors_event_t event;


if ((millis() - lastMsec) > 1000) {
    lastMsec = millis() ;

    ++masterCounter;

    if (masterCounter > 28000) {
      masterCounter = 0 ;
      while(1);
    }
      
    // Regularly "kick" the watchdog to prevent a system reset
    watchdog_update();
    // Regularly check in with Mqtt server
    client.loop();
    
    if (IMU.temperatureAvailable()) {
        IMU.readTemperature(temperature_deg);
        Serial.print("LSM6DSOX Temperature = ");
        Serial.print(temperature_deg);
        Serial.println(" °C");
    }
    dht.temperature().getEvent(&event);
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    temp = event.temperature; // Gets the values of the temperature
    dht.humidity().getEvent(&event);
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    hum = event.relative_humidity; // Gets the values of the humidity
    duration = (sonar.ping_median(20, 275));  // returns duration in microseconds
    Serial.print("Duration: ");
    Serial.print(duration);
    Serial.println(" uSeconds");
    rawdistance = sonar.convert_cm(duration); // Send ping, get distance in cm and print result (0 = outside set distance range)
    Serial.print("Ping: ");
    Serial.print(rawdistance); // Send ping, get distance in cm and print result (0 = outside set distance range)
    Serial.println(" cm (raw)");
  
    speedOfSound = 331.4 + (0.6 * temp) + (0.0124 * hum); // Calculate speed of sound in m/s
    Serial.print("Speed of Sound: ");
    Serial.print(speedOfSound);
    Serial.println("m/s");

    distance = (speedOfSound * (duration/2));
    distance = distance/10000. ; // meters to centimeters
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    memcpy(&tankgal_data_payload[0], &distance, sizeof(float));
    memcpy(&tankgal_data_payload[2], &hum, sizeof(float));
    memcpy(&tankgal_data_payload[4], &temp, sizeof(float));

    tankgal_data_payload[10] = temperature_deg ;  
    tankgal_data_payload[12] = masterCounter;
    tankgal_data_payload[16] = ErrCount;
    tankgal_data_payload[17] = ErrState;
    tankgal_data_payload[18] = 0;
    tankgal_data_payload[19] = 0;  

    client.publish(TANKGAL_CLIENT, (byte *)tankgal_data_payload, TANKGAL_LEN*4);


  const size_t capacity = JSON_OBJECT_SIZE(10);
  StaticJsonDocument<capacity> jsonDoc;

  jsonDoc["Distance to Water"] = distance;
  jsonDoc["Tank Humidity"] = hum;
  jsonDoc["Tank Temperature C"] = temp;
  jsonDoc["System Temp"] = temperature_deg;
  jsonDoc["Counter"] = masterCounter;
  jsonDoc["ErrCount"] = ErrCount;
  jsonDoc["ErrState"] = ErrState;

  char jsonBuffer[256];
  size_t n = serializeJson(jsonDoc, jsonBuffer);

  client.publish("TankGal JSON", jsonBuffer, n);

    
    Serial.print("Tank Gallon Data: ");
    for (i = 0; i <= 20; ++i)
    {
      Serial.print(tankgal_data_payload[i]);
      Serial.print(" ");
    }
    
    Serial.println();

  /*
  * Check Connection and Log State then Determine if a Reset is necessary
  */

    if (client.connected() == FALSE) {
      ErrState = client.state() ;
      ++ErrCount;
      Serial.print("Tank Gallon Monitor Disconnected from MQTT:");
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
