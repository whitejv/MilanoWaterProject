#include <stdio.h>
#include <string.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <IPAddress.h>
#include <PubSubClient.h>
#include <Arduino_LSM6DSOX.h>
#include <water.h>

PROGMEM int LastErrCode;
PROGMEM int LastErrCount;

char ssid[] = "ATT9LCV8fL_2.4";   // local wifi network SSID
char password[] = "6jhz7ai7pqy5"; // local network password

int InitiateReset = 0;
int ErrState = 0 ;
int ErrCount = 0 ;
#define ERRMAX 10 
#define firmwareVer 0x8004

IPAddress MQTT_BrokerIP(192, 168, 1, 249);
const char *mqttServer = "raspberrypi.local";
const int mqttPort = 1883;

// const char *mqttServer = "soldier.cloudmqtt.com";
// const int mqttPort = 15599;
// const char *mqttUser = "zerlcpdf";
// const char *mqttPassword = "OyHBShF_g9ya";

WiFiClient espWellClient;

PubSubClient client(MQTT_BrokerIP, mqttPort, espWellClient);

unsigned int masterCounter = 0;


/*
 * Data Block Interface Control
 */


void setup()
{
  digitalWrite(LEDR, HIGH);
  //Initialize //Serial and wait for port to open:
  Serial.begin(115200);
  /*
  while (!//Serial) {
    ; // wait for //Serial port to connect. Needed for native USB port only
  }
*/
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.print("Communication with WiFi module failed!");
    digitalWrite(LEDR, HIGH);
    while (true);
  }
/*
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    //Serial.print("Please upgrade the firmware");
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
  Serial.print(" WiFi connected -- ");
  Serial.print("Local IP address: ");
  Serial.println(WiFi.localIP());
   digitalWrite(LEDG, HIGH);  
  client.setServer(MQTT_BrokerIP, mqttPort);

  while (!client.connected())
  {
    Serial.print("Connecting to MQTT Server.....");
  
    if (client.connect(WELL_CLIENTID))
    {
      Serial.println("connected\n");
      digitalWrite(LEDB, HIGH);
    }
    else
    {
      Serial.print("failed with ");
      Serial.println("client state. ");
      Serial.println(client.state());
      delay(2000);
    }
  }
  //client.subscribe("ESP Control");
  
  if (!IMU.begin()) {  
    //Serial.println("Failed to initialize IMU!");
    while (1);
  }
  /*
   * Setup Pin Modes for Discretes
   */

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);   
}
void(* resetFunc) (void) = 0; //declare reset function @ address 0
void loop()
{
  int i;
  int decimal;
  int temperature_deg = 0;

  ++masterCounter;

  if (masterCounter > 28000) {
    masterCounter = 0 ;
    //resetFunc();  //call reset
  }
  
  if (IMU.temperatureAvailable())
  {

    IMU.readTemperature(temperature_deg);
    Serial.print("LSM6DSOX Temperature = ");
    Serial.print(temperature_deg);
    Serial.println(" °C");
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
  well_data_payload[18] = LastErrCount;
  well_data_payload[19] = LastErrCode;  

  client.loop();

  client.publish(WELL_CLIENT, (byte *)well_data_payload, WELL_LEN*4);
  
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
    //Write Error State to Flash
    LastErrCode = ErrState;
    ++LastErrCount;
    //Initiate Reset
    Serial.println("Initiate board reset!!") ;
    //resetFunc();  //call reset
  }
    
  delay(1200);

}
