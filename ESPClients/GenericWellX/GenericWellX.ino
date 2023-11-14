#include <stdio.h>
#include <string.h>
#include <SPI.h>
#include <WiFiS3.h>
#include <Wire.h>
#include <IPAddress.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WDT.h>
#include <water.h>

int InitiateReset = 0;
int ErrState = 0 ;
int ErrCount = 0 ;

//3 seconds WDT
#define WDT_TIMEOUT 3000
const long wdtInterval = WDT_TIMEOUT;
unsigned long wdtMillis = 0;


#define ERRMAX 10 
#define firmwareVer 0x8004

WiFiClient espWellClient;
PubSubClient P_client(espWellClient);
PubSubClient D_client(espWellClient);
PubSubClient client; // Declare the client object globally

unsigned int masterCounter = 0;


void setup()
{
  //digitalWrite(LEDR, HIGH);
  //Initialize Serial
  Serial.begin(115200);

 
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.print("Communication with WiFi module failed!");
    //digitalWrite(LEDR, HIGH);
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
  //digitalWrite(LEDG, HIGH);
  
unsigned long connectAttemptStart = millis();
bool connected = false;

//Try connecting to the production MQTT server first

P_client.setServer(PROD_MQTT_IP, PROD_MQTT_PORT);

// Connect to the MQTT server

while (!P_client.connected() && millis() - connectAttemptStart < 5000) { // Adjust the timeout as needed
  Serial.print("Connecting to Production MQTT Server: ...");
  connected = P_client.connect(WELL_CLIENTID);
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
  D_client.setServer(DEV_MQTT_IP, DEV_MQTT_PORT);
  while (!D_client.connected()) {
    Serial.print("Connecting to Development MQTT Server...");
    connected = D_client.connect(WELL_CLIENTID);
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
  
  Serial.print("Configuring WDT...");
   if(wdtInterval < 1) {
    Serial.println("Invalid watchdog interval");
    while(1){}
  }

  if(WDT.begin(wdtInterval)) {
    Serial.print("WDT interval: ");
    WDT.refresh();
    Serial.print(WDT.getTimeout());
    WDT.refresh();
    Serial.println(" ms");
    WDT.refresh();
  } else {
    Serial.println("Error initializing watchdog");
    while(1){}
  }


  /*
   * Setup Pin Modes for Discretes and increase analogs to 12bits
   */

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  analogReadResolution(14);   
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
    if(millis() - wdtMillis >= wdtInterval - 1) {
    WDT.refresh(); // Comment this line to stop refreshing the watchdog
    wdtMillis = millis();
  }

  well_data_payload[0] = analogRead(A0);
  well_data_payload[1] = analogRead(A1);
  well_data_payload[2] = analogRead(A2);
  well_data_payload[3] = analogRead(A3);
  well_data_payload[4] = analogRead(A4);
  well_data_payload[5] = digitalRead(2);
  well_data_payload[6] = digitalRead(3);
  well_data_payload[10] = temperature_deg ;  
  well_data_payload[12] = masterCounter;
  well_data_payload[16] = ErrCount;
  well_data_payload[17] = ErrState;
  well_data_payload[18] = 0;
  well_data_payload[19] = 0;  

  client.loop();

  client.publish(WELLSENS_TOPICID, (byte *)well_data_payload, WELL_LEN*4);

const size_t capacity = JSON_OBJECT_SIZE(10);
StaticJsonDocument<capacity> jsonDoc;

jsonDoc["A0"] = analogRead(A0);
jsonDoc["A1"] = analogRead(A1);
jsonDoc["A2"] = analogRead(A2);
jsonDoc["A3"] = analogRead(A3);
//jsonDoc["A7"] = analogRead(A7);
jsonDoc["D2"] = digitalRead(2);
jsonDoc["D3"] = digitalRead(3);
jsonDoc["Temp"] = temperature_deg;
jsonDoc["Counter"] = masterCounter;
jsonDoc["ErrCount"] = ErrCount;
jsonDoc["ErrState"] = ErrState;

char jsonBuffer[256];
size_t n = serializeJson(jsonDoc, jsonBuffer);

client.publish(WELLSENS_JSONID, jsonBuffer, n);

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
