#include <stdio.h>
#include <string.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <IPAddress.h>
#include <PubSubClient.h>

char ssid[] = "ATT9LCV8fL_2.4"; //local wifi network SSID
char pass[] = "6jhz7ai7pqy5";   //local network password

#define MCP3428Addr  0x68
#define MCP3428XAddr 0x6f
#define TMP100Addr   0x4f
#define MCP23008Addr 0x20
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

/*
 * Client app for reading Raspberry PI GPIO signals
 * and transferring them via MQTT to a subscriber app.
 *
 */

/*
 * Data Block Interface Control
 */

/*
 * Data Word 0:  CH1: Dead - Damaged
 * Data Word 1:  CH2: Raw Sensor Current Sense Well 1 16bit
 * Data Word 2:  CH3: Raw Sensor Current Sense Well 2 16bit
 * Data Word 3:  CH4: Raw Sensor Current Sense Well 3 16bit
 * Data Word 4:  GPIO 8 bits Hex (8 bits: 0-3 floats, 4 Pump Relay Command)
 * Data Word 5:  Raw Temp Celcius
 * Data Word 6:  Flow Sensor Count 16bit Int
 * Data Word 7:  Flow Sensor Period 16bit Int
 * Data Word 8:  CH1: 4-20 mA Raw Sensor HydroStatic Pressure 16bit
 * Data Word 9:  CH2: Raw Sensor House Water Pressure 16bit ADC 0-5v
 * Data Word 10: CH3: Unused 2 16bit
 * Data Word 11: CH4: Raw Sensor Current Sense Irrigation pump 4 (16bit)
 * Data Word 12: Cycle Counter 16bit Int
 * Data Word 13: spare
 * Data Word 14: spare
 * Data Word 15: spare
 * Data Word 16: I2C Panic Count 16bit Int
 * Data Word 17: TMP100 I2C Error
 * Data Word 18: MCP23008 I2C Error
 * Data Word 19: MCP3428 I2C Error
 * Data Word 20: FW Version 4 Hex 
 */
/*
 * Data Word 21: Last Data Word is Control Word From User
 */

unsigned short int raw_sensor_data[22] = {0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, firmwareVer,
                                          0};
unsigned int pumpPower;
unsigned int GPIOControlWord = 0;

void I2CComm()

{
   int i;
   unsigned int data[2];

   /*
    * Read TMP100
    */

   Wire.beginTransmission(TMP100Addr); // Start I2C Transmission
   I2CPanic = Wire.endTransmission();  // Stop I2C Transmission

   if (I2CPanic != 0)
   {
      raw_sensor_data[17] = I2CPanic;
      ++I2CPanicCount;
   }

   delay(100);
   Wire.requestFrom(TMP100Addr, 2); // Request 2 bytes of data

   // Read 2 bytes of data raw_adc msb, raw_adc lsb

   if (Wire.available() == 2)
   {
      data[0] = Wire.read();
      data[1] = Wire.read();
      raw_sensor_data[5] = (data[0] << 8) | data[1];
      raw_sensor_data[5] = raw_sensor_data[5] >> 4;
   }
   else
   {
      raw_sensor_data[17] = 5;
   }

   /*
   * Read MCP23008
   */

   Wire.beginTransmission(MCP23008Addr); // Start I2C Transmission
   Wire.write(0x09);                     // Selects the GPIO pin register

   I2CPanic = Wire.endTransmission(); // Stop I2C Transmission

   if (I2CPanic != 0)
   {
      raw_sensor_data[18] = I2CPanic;
      ++I2CPanicCount;
   }

   delay(100);

   Wire.requestFrom(MCP23008Addr, 1); // Request 2 bytes of data

   // Read 1 byte of data

   if (Wire.available() == 1)
   {
      data[0] = Wire.read();
      //printf("GPIO Data: %x\n", data[0]);
      raw_sensor_data[4] = data[0];
   }
   else
   {
      raw_sensor_data[18] = 5;
   }

   /*
    * Read MCP3428 Channel 1-4
    */

   static unsigned short int ChannelCommand[4] = {0x88, 0xB8, 0xD8, 0xF8};

   for (i = 0; i <= 3; ++i)
   {
      data[0] = 0;
      data[1] = 0;

      Wire.beginTransmission(MCP3428Addr); // Start I2C Transmission
      Wire.write(ChannelCommand[i]);       // Select data register

      I2CPanic = Wire.endTransmission(); // Stop I2C Transmission

      if (I2CPanic != 0)
      {
         raw_sensor_data[19] = I2CPanic;
         printf("%d\n", I2CPanic);
         ++I2CPanicCount;
      }

      delay(100);

      Wire.requestFrom(MCP3428Addr, 3); // Request 2 bytes of data

      //printf("%d\n", Wire.available());
      // Read 2 bytes of data  raw_adc msb, raw_adc lsb

      if (Wire.available() == 3)
      {
         data[0] = Wire.read();
         data[1] = Wire.read();
         raw_sensor_data[i] = (data[0] << 8) | data[1];
      }
      else
      {
         raw_sensor_data[19] = 5;
      }
   }
   for (i = 0; i <= 3; ++i)
   {
      data[0] = 0;
      data[1] = 0;

      Wire.beginTransmission(MCP3428XAddr); // Start I2C Transmission
      Wire.write(ChannelCommand[i]);        // Select data register

      I2CPanic = Wire.endTransmission(); // Stop I2C Transmission

      if (I2CPanic != 0)
      {
         raw_sensor_data[19] = I2CPanic;
         printf("%d\n", I2CPanic);
         ++I2CPanicCount;
      }

      delay(100);

      Wire.requestFrom(MCP3428XAddr, 3); // Request 2 bytes of data

      //printf("%d\n", Wire.available());
      // Read 2 bytes of data  raw_adc msb, raw_adc lsb

      if (Wire.available() == 3)
      {
         data[0] = Wire.read();
         data[1] = Wire.read();
         raw_sensor_data[8 + i] = (data[0] << 8) | data[1];
      }
      else
      {
         raw_sensor_data[19] = 5;
      }
   }
}

void callback(char *topic, byte *payload, unsigned int length)
{

   printf("Message arrived in topic: %s  Length: %d\n", topic, length);

   for (int i = 0; i < length; i++)
   {
      printf("%x ", payload[i]);
   }

   GPIOControlWord = payload[0];
   printf("GPIO: %x\n", GPIOControlWord);

   Wire.beginTransmission(MCP23008Addr); // Start I2C Transmission
   Wire.write(0x09);                     //Selects the GPIO pin register
   Wire.write(GPIOControlWord);

   I2CPanic = Wire.endTransmission(); // Stop I2C Transmission

   if (I2CPanic != 0)
   {

      raw_sensor_data[11] = I2CPanic;

      ++I2CPanicCount;
   }
}

void setup()
{

   int ResolutionBits = 10; //Resolution set

   Serial.begin(115200);

   WiFi.begin(ssid, pass); // Connect to WiFi network

   delay(20);

   while (WiFi.status() != WL_CONNECTED)
   {
      delay(500);
   }

   //WifiStatus = WiFi.status();

   // Initialise I2C communication as MASTER and configure MCP3428

   Wire.begin(12, 14);

   // Wire.setClock(10000);

   Wire.beginTransmission(MCP3428Addr); // Start I2C Transmission
   Wire.write(0x98);
   Wire.endTransmission(); // Stop I2C Transmission

   Wire.beginTransmission(MCP3428XAddr); // Start I2C Transmission
   Wire.write(0x98);
   Wire.endTransmission(); // Stop I2C Transmission

   // Initialize the TMP100

   Wire.beginTransmission(TMP100Addr);
   Wire.write(B00000001);                 //addresses the configuration register
   Wire.write((ResolutionBits - 9) << 5); //writes the resolution bits
   Wire.endTransmission();

   Wire.beginTransmission(TMP100Addr); //resets to reading the temperature
   Wire.write((byte)0x00);
   Wire.endTransmission();

   // Start I2C Transmission with mcp23008

   Wire.beginTransmission(MCP23008Addr);
   Wire.write(0x00);       //Selects the IODIRA Register
   Wire.write(0b00011111); //0 Set the first 4 as Input and the next 4 as Output
   Wire.write(0b00011111); //1 Set the first 4 as Input and the next 4 as Output
   Wire.write(0b00000000); //2 Set the first 4 as Input and the next 4 as Output
   Wire.write(0b00000000); //3 Set the first 4 as Input and the next 4 as Output
   Wire.write(0b00000000); //4 Set the first 4 as Input and the next 4 as Output
   Wire.write(0b00000000); //5 Set the first 4 as Input and the next 4 as Output
   Wire.write(0b00011111); //6 Set the first 4 as Input and the next 4 as Output
   Wire.write(0b00000000); //7 Set the first 4 as Input and the next 4 as Output
   Wire.write(0b00000000); //8 Set the first 4 as Input and the next 4 as Output
   Wire.write(0b00000000); //9 Set the first 4 as Input and the next 4 as Output
   Wire.write(0b00000000); //A Set the first 4 as Input and the next 4 as Output

   Wire.endTransmission(); // Stop I2C Transmission

   client.setServer(MQTT_BrokerIP, mqttPort);
   client.setCallback(callback);

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

   I2CComm();

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
   printf("%x  \n", GPIOControlWord);
   // printf("|| Frame Rate: %4.5f", frameRate);
   // printf("\n");

   delay(400);
}
