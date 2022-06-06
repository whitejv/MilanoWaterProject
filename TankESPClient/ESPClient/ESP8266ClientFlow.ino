#include <stdio.h>
#include <string.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <IPAddress.h>
#include <PubSubClient.h>

char ssid[] = "ATT9LCV8fL_2.4"; //local wifi network SSID
char pass[] = "6jhz7ai7pqy5";   //local network password

#define firmwareVer  0x8002

IPAddress MQTT_BrokerIP(192, 168, 1, 154);
const char *mqttServer = "raspberrypi.local";
const int mqttPort = 1883;

//const char *mqttServer = "soldier.cloudmqtt.com";
//const int mqttPort = 15599;
//const char *mqttUser = "zerlcpdf";
//const char *mqttPassword = "OyHBShF_g9ya";

WiFiClient espClient;

PubSubClient client(MQTT_BrokerIP, mqttPort, espClient);

unsigned int masterCounter = 0;

int I2CPanicCount = 0;
int I2CPanic = 0;
float calibrationFactor = 4.5;
volatile byte pulseCount = 0;  
float flowRate = 0.0;
unsigned int flowMilliLitres = 0;
float flowGallons = 0;
unsigned long totalMilliLitres = 0;
unsigned long oldTime = 0;
/*
 *Liquid flow rate sensor -DIYhacking.com Arvind Sanjeev
 *
 *Measure the liquid/water flow rate using this code. 
 *Connect Vcc and Gnd of sensor to arduino, and the 
 *signal line to arduino digital pin 2.
 */

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.

int HallFlowSensorPin = 2;

/*
 * Client app for reading Raspberry PI GPIO signals
 * and transferring them via MQTT to a subscriber app.
 *
 */

/*
 * Data Block Interface Control
 */

/*
 * Data Word 0:  Gallons Per Minute
 * Data Word 1:  Pump Temperature - analog 16bit
 * Data Word 2:  Pump PSI - analog 16bit
 * Data Word 3:  spare
 * Data Word 4:  spare
 */


unsigned short int raw_sensor_data[22] = {0, 0, 0, 0, 0};

IRAM_ATTR void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
void ReadFlowSensorCallback()
{
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(digitalPinToInterrupt(HallFlowSensorPin));
    printf("Pulse Count = %d\n", pulseCount);
    // pulseCount = 8230;  //testing only    
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    flowRate = flowRate * .00026417 ;
    printf("Flow Rate: %f\n", flowRate);
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    flowGallons = flowMilliLitres * .00026417;
    
    // Add the millilitres passed in this second to the cumulative total
    //totalMilliLitres += flowMilliLitres;
    
  
  
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
      attachInterrupt(digitalPinToInterrupt(HallFlowSensorPin), pulseCounter, FALLING);
}
void INTCommCallback()
{   
   /*** Disable the interrupt while calculating     ***/
   /*** flow rate and sending the value to the host ***/

    detachInterrupt(digitalPinToInterrupt(HallFlowSensorPin));
    printf("Pulse Count = %d\n", pulseCount);
    
    raw_sensor_data[6]  = pulseCount;
    pulseCount = 0;   // Reset the pulse counter so we can start incrementing again
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(digitalPinToInterrupt(HallFlowSensorPin), pulseCounter, FALLING);
}


void setup()
{


   Serial.begin(115200);

   WiFi.begin(ssid, pass); // Connect to WiFi network

   delay(20);

   while (WiFi.status() != WL_CONNECTED)
   {
      delay(500);
   }

   //WifiStatus = WiFi.status();

  
  // The Hall-effect sensor is connected to pin 04.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)

   attachInterrupt(digitalPinToInterrupt(HallFlowSensorPin), pulseCounter, FALLING);

   client.setServer(MQTT_BrokerIP, mqttPort);
   //client.setCallback(callback);

   while (!client.connected())
   {
      printf("Connecting to MQTT.....");
      //if (client.connect("ESP8266Client", mqttUser, mqttPassword))
      if (client.connect("ESP8266Client"))
      {
         printf("connected\n");
      }
      else
      {
         printf("failed with ");
         printf("client state %d\n", client.state());
         delay(2000);
      }
   }

   client.subscribe("ESP Control");
}

void loop()
{
   int i;
   int decimal;

   ++masterCounter;

   if (masterCounter == 32000)
   {
      masterCounter = 0;
   }
   raw_sensor_data[12] = masterCounter;
   raw_sensor_data[16] = I2CPanicCount;

   client.loop();

   client.publish("Tank ESP", (byte *)raw_sensor_data, 42);

   printf("ESP Data: ");
   for (i = 0; i <= 16; ++i)
   {
      printf("%x ", raw_sensor_data[i]);
   }
   for (i = 17; i <= 19; ++i)
   {
      decimal = (short)raw_sensor_data[i];
      printf("%d ", decimal);
   }
   for (i = 20; i <= 20; ++i)
   {
      printf("%x ", raw_sensor_data[i]);
   }
  
   // printf("|| Frame Rate: %4.5f", frameRate);
   // printf("\n");

   delay(400);
}
