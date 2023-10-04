#include <stdio.h>
#include <string.h>
#include <SPI.h>
#include <WiFiS3.h>
//#include <WiFi.h>
#include <Wire.h>
#include <IPAddress.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Arduino_LSM6DSOX.h>
//#include "hardware/watchdog.h" // Include the watchdog header
#include <water.h>

//char ssid[] = "ATT9LCV8fL_2.4";   // local wifi network SSID
//char password[] = "6jhz7ai7pqy5"; // local network password

int InitiateReset = 0;
int ErrState = 0 ;
int ErrCount = 0 ;

// Watchdog timeout period in milliseconds
const int watchdogTimeoutMs = 3000; // 6 seconds, adjust as needed

#define ERRMAX 10 
#define firmwareVer 0x8004

WiFiClient espWellClient;

/* Define the IPs for Production and Development MQTT Servers */
//IPAddress prodMqttServerIP(192, 168, 1, 250);
//IPAddress devMqttServerIP(192, 168, 1, 249) ;
IPAddress prodMqttServerIP(192, 168, 0, 84);
IPAddress devMqttServerIP(192, 168, 0, 84) ;
PubSubClient P_client(espWellClient);
PubSubClient D_client(espWellClient);


PubSubClient client; // Declare the client object globally
unsigned int masterCounter = 0;


/*
 * Data Block Interface Control
 */


void setup()
{
  digitalWrite(LED_RED, HIGH);
  //Initialize Serial
  Serial.begin(115200);

 
  // check for the WiFi module:
  /*
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.print("Communication with WiFi module failed!");
    digitalWrite(LED_RED, HIGH);
    while (true);
  }
 */
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
  digitalWrite(LED_GREEN, HIGH);
  
unsigned long connectAttemptStart = millis();
bool connected = false;

//Try connecting to the production MQTT server first

P_client.setServer(prodMqttServerIP, PROD_MQTT_PORT);

// Connect to the MQTT server

while (!P_client.connected() && millis() - connectAttemptStart < 5000) { // Adjust the timeout as needed
  Serial.print("Connecting to Production MQTT Server: ...");
  connected = P_client.connect(WELL_CLIENTID);
  if (connected) {
    client = P_client; // Assign the connected production client to the global client object
    Serial.println("connected\n");
    digitalWrite(LED_BLUE, HIGH);
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
    connected = D_client.connect(WELL_CLIENTID);
    if (connected) {
      client = D_client; // Assign the connected development client to the global client object
      Serial.println("connected\n");
      digitalWrite(LED_BLUE, HIGH);
    } else {
      Serial.print("failed with client state: ");
      printClientState(D_client.state());
      Serial.println();
      delay(2000);
    }
  }
}


  //client.subscribe("ESP Control");
  
  // Enable the watchdog timer
  // watchdog_enable(watchdogTimeoutMs, 1);
  // Serial.println("Watchdog Timer Enabled\n");

  //if (!IMU.begin()) {  
    //Serial.println("Failed to initialize IMU!");
    //while (1);
  //}
  /*
   * Setup Pin Modes for Discretes and increase analogs to 12bits
   */

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  analogReadResolution(12);   
}

void loop()
{
  int i;
  int decimal;
  int temperature_deg = 0;

  ++masterCounter;

  if (masterCounter > 28000) {
    masterCounter = 0 ;
    while(1);
  }
    
  // Regularly "kick" the watchdog to prevent a system reset
  // watchdog_update();

  if (IMU.temperatureAvailable())
  {

    IMU.readTemperature(temperature_deg);
    //Serial.print("LSM6DSOX Temperature = ");
    //Serial.print(temperature_deg);
    //Serial.println(" °C");
  }

  well_data_payload[0] = analogRead(A0);
  well_data_payload[1] = analogRead(A1);
  well_data_payload[2] = analogRead(A2);
  well_data_payload[3] = analogRead(A3);
  well_data_payload[4] = analogRead(A7);
  well_data_payload[5] = digitalRead(2);
  well_data_payload[6] = digitalRead(3);
  well_data_payload[10] = temperature_deg ;  
  well_data_payload[12] = masterCounter;
  well_data_payload[16] = ErrCount;
  well_data_payload[17] = ErrState;
  well_data_payload[18] = 0;
  well_data_payload[19] = 0;  

  client.loop();

  client.publish(WELL_CLIENT, (byte *)well_data_payload, WELL_LEN*4);

const size_t capacity = JSON_OBJECT_SIZE(10);
StaticJsonDocument<capacity> jsonDoc;

jsonDoc["A0"] = analogRead(A0);
jsonDoc["A1"] = analogRead(A1);
jsonDoc["A2"] = analogRead(A2);
jsonDoc["A3"] = analogRead(A3);
jsonDoc["A7"] = analogRead(A7);
jsonDoc["D2"] = digitalRead(2);
jsonDoc["D3"] = digitalRead(3);
jsonDoc["Temp"] = temperature_deg;
jsonDoc["Counter"] = masterCounter;
jsonDoc["ErrCount"] = ErrCount;
jsonDoc["ErrState"] = ErrState;

char jsonBuffer[256];
size_t n = serializeJson(jsonDoc, jsonBuffer);

client.publish("Well JSON", jsonBuffer, n);

  Serial.print("Well Pump Data: ");
  for (i = 0; i <= 20; ++i)
  {
    Serial.print(well_data_payload[i]);
    Serial.print(" ");
  }
  
  Serial.println();

/*
 * Check Connection and Log State then Determine if a Reset is necessary
 */

  if (client.connected() == FALSE) {
    ErrState = client.state() ;
    ++ErrCount;
    Serial.print("Well Monitor Disconnected from MQTT:");
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
    
  delay(1000);

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
