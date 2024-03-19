#include <stdio.h>
#include <string.h>
#include <SPI.h>
#include <WiFiNINA.h>

//#include <WiFiS3.h>
//#include <WiFi.h>
#include <Wire.h>
#include <IPAddress.h>
#include <ArduinoJson.h>

//#include <cJSON.h>

#include <PubSubClient.h>
#include <Arduino_LSM6DSOX.h>
#include "hardware/watchdog.h" // Include the watchdog header
#include <water.h>

int InitiateReset = 0;
int ErrState = 0 ;
int ErrCount = 0 ;
int extendedSensor = 0;
int sensor = 0;
char messageName[50];
char messageNameJSON[50];

// Watchdog timeout period in milliseconds
const int watchdogTimeoutMs = 3000; // 6 seconds, adjust as needed
unsigned int masterCounter = 0;

#define ERRMAX 10 
#define firmwareVer 0x8004

WiFiClient espWellClient;

//Define the IPs for Production and Development MQTT Servers
IPAddress prodMqttServerIP(192, 168, 1, 250);
IPAddress devMqttServerIP(192, 168, 1, 249) ;
PubSubClient P_client(espWellClient);
PubSubClient D_client(espWellClient);

PubSubClient client; // Declare the client object globally

void setup()
{
  
  //Initialize Serial
  Serial.begin(115200);
 
 /*
  * Important - Uncomment for USB serial port development
  *             comment out for production
  */

// while (!Serial) {
//   ; // wait for serial port to connect. Needed for native USB port only
//  }


  // check for the WiFi module:

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.print("Communication with WiFi module failed!");
    digitalWrite(LEDR, HIGH);
    while (true);
  }
 
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
   
  // Sensors 0-3 are standard sensors
  // Sensors 4-7 are extended sensors with additional data words
  
  sensor = 4 ;
  Serial.print(" #");
  Serial.print(sensor);
  if (sensor >= 4){ 
    extendedSensor = 1;
    Serial.print(" Extended ") ;
  }
  Serial.print(" Sensor ID: ");
  Serial.println(flowSensorConfig[sensor].sensorName);

  
  strcpy(messageName, flowSensorConfig[sensor].messageid);
  strcpy(messageNameJSON, flowSensorConfig[sensor].jsonid);
unsigned long connectAttemptStart = millis();
bool connected = false;

//Try connecting to the production MQTT server first

P_client.setServer(prodMqttServerIP, PROD_MQTT_PORT);


// Connect to the MQTT server

while (!P_client.connected() && millis() - connectAttemptStart < 5000) { // Adjust the timeout as needed
  Serial.print("Connecting to Production MQTT Server: ...");
  connected = P_client.connect(flowSensorConfig[sensor].clientid);;
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
  //PubSubClient client(devMqttServerIP, DEV_MQTT_PORT, espWellClient);
  D_client.setServer(devMqttServerIP, DEV_MQTT_PORT);
  while (!D_client.connected()) {
    Serial.print("Connecting to Development MQTT Server...");
    connected = D_client.connect(flowSensorConfig[sensor].clientid);
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

// Set internal MQTT buffer size
  if (!client.setBufferSize (1024) ) {
     Serial.println("Warning - MQTT buffer too small for JSON data.");
  }

  //client.subscribe("ESP Control");
  
  // Enable the watchdog timer
   watchdog_enable(watchdogTimeoutMs, 1);
   Serial.println("Watchdog Timer Enabled\n");

  if (!IMU.begin()) {  
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

// Setup Pin Modes for Discretes and increase analogs to 12bits
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  analogReadResolution(12); 
 
}

void loop()
{
  
  int i;
  int decimal;
  int status;
  float temperature_deg = 0;

  ++masterCounter;

  if (masterCounter > 28000) {
    masterCounter = 0 ;
    while(1);
  }
    
  // Regularly "kick" the watchdog to prevent a system reset
 watchdog_update();

  if (IMU.temperatureAvailable())
  {

    IMU.readTemperatureFloat(temperature_deg);
    temperature_deg = temperature_deg * 9.0 / 5.0 + 32;
    //Serial.print("LSM6DSOX Temperature = ");
    //Serial.print(temperature_deg);
    //Serial.println(" °F");
  }

    genericSens_.generic.milliseconds = 0   ;
    genericSens_.generic.new_data_flag = 0  ;
    genericSens_.generic.adc_sensor = 0   ;
    genericSens_.generic.gpio_sensor = 0   ;
    genericSens_.generic.temp = (int)temperature_deg;
    memcpy(&genericSens_.generic.temp_w1,  &temperature_deg, sizeof(temperature_deg));
    genericSens_.generic.cycle_count = masterCounter   ;
    genericSens_.generic.fw_version = 1111   ;
    genericSens_.generic.adc_x1 = analogRead(A0);
    genericSens_.generic.adc_x2 = analogRead(A1);
    genericSens_.generic.adc_x3 = analogRead(A2);
    genericSens_.generic.adc_x4 = analogRead(A3);
    genericSens_.generic.adc_x5 = 0;
    genericSens_.generic.adc_x6 = 0;
    genericSens_.generic.adc_x7 = 0;
    genericSens_.generic.adc_x8 = 0;
    genericSens_.generic.GPIO_x1 = (digitalRead(2) <<1) | digitalRead(3);
    genericSens_.generic.GPIO_x2 = 0;

  client.loop();

/*
  Serial.print("S005D Well Data: ");
  for (i = 0; i <= 20; ++i)
  {
    Serial.print(well_data_payload[i]);
    Serial.print(" ");
  }
  
  Serial.println();
*/

  client.publish(flowSensorConfig[sensor].messageid, (byte *)genericSens_.data_payload, flowSensorConfig[sensor].messagelen*4);
 
 //
 const size_t capacity = JSON_OBJECT_SIZE(40) + 40 * JSON_OBJECT_SIZE(2);
StaticJsonDocument<capacity> jsonDoc;

for (int i = 0; i < flowSensorConfig[sensor].messagelen; i++) {
    jsonDoc[genericsens_ClientData_var_name[i]] = genericSens_.data_payload[i];
}

char jsonBuffer[1024];
size_t n = serializeJson(jsonDoc, jsonBuffer);
status = client.publish(flowSensorConfig[sensor].jsonid, jsonBuffer, n);
if (status == 0) {
    Serial.print("Failed to publish JSON message. Status: ");
    Serial.println(status);
}
 
  //If you are using cJSON
  // Create a new cJSON object
  //cJSON *jsonDoc = cJSON_CreateObject();

  //Serial.printf("message length   %d", flowSensorConfig[sensor].messagelen);
  //for (i = 0; i < flowSensorConfig[sensor].messagelen; i++) {
  //    // Add data to the cJSON object
  //    cJSON_AddNumberToObject(jsonDoc, genericsens_ClientData_var_name[i], genericSens_.data_payload[i]);
  //}

  // Serialize the cJSON object to a string
  //char *jsonBuffer = cJSON_Print(jsonDoc);
  //if (jsonBuffer != NULL) {
  //    size_t n = strlen(jsonBuffer);
      //Serial.printf(flowSensorConfig[sensor].jsonid);
      //Serial.printf("   %d", n);
      //Serial.printf("\n");
      //Serial.printf(jsonBuffer);
      //Serial.printf("\n");
      
      // Publish the JSON data
  //   if (client.publish(flowSensorConfig[sensor].jsonid, jsonBuffer, n) == FALSE) {
  //      Serial.println("JSON Message Failed to Publish");
  //    }
      // Free the serialized data buffer
  //    free(jsonBuffer);
 // }

  // Delete the cJSON object
  //cJSON_Delete(jsonDoc);

  Serial.print(messageName);
  Serial.print(":  ");
  //Serial.printf("message length   %d",flowSensorConfig[sensor].messagelen); 
  for (int i = 0; i<=flowSensorConfig[sensor].messagelen-1; ++i) {
    Serial.print(genericSens_.data_payload[i]);
    Serial.print(" ");
  }
  Serial.println("");


 //Check Connection and Log State then Determine if a Reset is necessary
 

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
