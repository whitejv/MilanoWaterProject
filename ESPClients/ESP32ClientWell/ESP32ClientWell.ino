#include <stdio.h>
#include <string.h>
#include <Wire.h>
#include <IPAddress.h>
#include <PubSubClient.h>

/* NCD ESP8266 Board - Select GENERIC ESP8266 MODULE
 * in Arduino Board Manager
 */
#if defined(ARDUINO_ESP8266_GENERIC) || defined(ARDUINO_ESP8266_WEMOS_D1MINI)
#include <ESP8266WiFi.h>
#endif
/* NCD ESP32 Board - Select Adafruit ESP32 Feather
 * in Arduino Board Manager
 */
#if defined(ARDUINO_FEATHER_ESP32)
#include <WiFi.h>
#include <esp_task_wdt.h>
#endif
/* RPI PICO W - Select ARDUINO_RASPBERRY_PI_PICO_W
 * in Arduino Board Manager
 */
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#endif

#define WDT_TIMEOUT 10 // 10 seconds Watch Dog Timer (WDT)

char ssid[] = "ATT9LCV8fL_2.4";   // local wifi network SSID
char password[] = "6jhz7ai7pqy5"; // local network password

#define MCP3428Addr 0x68
#define MCP3428XAddr 0x6f
#define TMP100Addr 0x4f
#define MCP23008Addr 0x20
#define firmwareVer 0x8004
#define ResolutionBits 10

IPAddress MQTT_BrokerIP(192, 168, 1, 249);
const char *mqttServer = "raspberrypi.local";
const int mqttPort = 1883;

// const char *mqttServer = "soldier.cloudmqtt.com";
// const int mqttPort = 15599;
// const char *mqttUser = "zerlcpdf";
// const char *mqttPassword = "OyHBShF_g9ya";

WiFiClient espClient;

PubSubClient client(MQTT_BrokerIP, mqttPort, espClient);

int last_I2CPanicCount = 0;
int WDT_Interval = 0;
unsigned int masterCounter = 0;
int I2CPanicCount = 0;
int I2CPanic = 0;

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

int raw_sensor_data[22] = {0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, firmwareVer,
                                          0};

void setup()
{

  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected -- ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(MQTT_BrokerIP, mqttPort);

  while (!client.connected())
  {
    Serial.printf("Connecting to MQTT.....");
    // if (client.connect("ESP8266Client", mqttUser, mqttPassword))
    if (client.connect("ESP8266Client"))
    {
      Serial.printf("connected\n");
    }
    else
    {
      Serial.printf("failed with ");
      Serial.printf("client state %d\n", client.state());
      delay(2000);
    }
  }
  client.subscribe("ESP Control");

#if defined(ARDUINO_FEATHER_ESP32)
  Serial.printf("Configuring WDT...");
  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);               // add current thread to WDT watch
  Serial.printf("Complete\n");
  Wire.begin();
#endif
#if defined(ARDUINO_ESP8266_GENERIC) || defined(ARDUINO_ESP8266_WEMOS_D1MINI)
  Wire.begin(12, 14);
#endif
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  i2c_init(i2c_default, 100 * 1000);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  // gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  // gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
  Wire.begin();
#endif

  // Wire.setClock(10000);

  Wire.beginTransmission(MCP3428Addr); // Start I2C Transmission
  Wire.write(0x98);
  Wire.endTransmission(); // Stop I2C Transmission

  Wire.beginTransmission(MCP3428XAddr); // Start I2C Transmission
  Wire.write(0x98);
  Wire.endTransmission(); // Stop I2C Transmission

  // Initialize the TMP100

  Wire.beginTransmission(TMP100Addr);
  Wire.write(0b00000001);                // addresses the configuration register
  Wire.write((ResolutionBits - 9) << 5); // writes the resolution bits
  Wire.endTransmission();

  Wire.beginTransmission(TMP100Addr); // resets to reading the temperature
  Wire.write((byte)0x00);
  Wire.endTransmission();

  // Start I2C Transmission with mcp23008

  Wire.beginTransmission(MCP23008Addr);
  Wire.write(0x00);       // Selects the IODIRA Register
  Wire.write(0b00111111); // 0 Set the first 6 as Input and the next 2 as Output
  Wire.write(0b00111111); // 1 Set the first 6 as Input and the next 2 as Output
  Wire.write(0b00000000); // 2
  Wire.write(0b00000000); // 3
  Wire.write(0b00000000); // 4
  Wire.write(0b00000000); // 5
  Wire.write(0b00111111); // 6 Set the first 6 as Input and the next 2 as Output
  Wire.write(0b00000000); // 7
  Wire.write(0b00000000); // 8
  Wire.write(0b00000000); // 9
  Wire.write(0b00000000); // A

  Wire.endTransmission(); // Stop I2C Transmission
}
void I2CComm()
{
  int i = 0, j = 0, offset = 0;
  static unsigned short int MCPaddress[] = {MCP3428Addr, MCP3428XAddr};
  static unsigned short int ChannelCommand[4] = {0x88, 0xB8, 0xD8, 0xF8};
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
    // Serial.printf("GPIO Data: %x\n", data[0]);
    raw_sensor_data[4] = data[0];
  }
  else
  {
    raw_sensor_data[18] = 5;
  }

  /*
   * Read Both MCP3428s Channel 1-4
   */

  for (j = 0; j <= 1; ++j)
  {
    for (i = 0; i <= 3; ++i)
    {
      data[0] = 0;
      data[1] = 0;

      Wire.beginTransmission(MCPaddress[j]); // Start I2C Transmission
      Wire.write(ChannelCommand[i]);         // Select data register
      I2CPanic = Wire.endTransmission();     // Stop I2C Transmission
      // Serial.printf("MCP3428: %0x Channel Command: %0x ", MCPaddress[j], ChannelCommand[i]);
      if (I2CPanic != 0)
      {
        raw_sensor_data[19] = I2CPanic;
        // Serial.printf("%d panic for MCP3428 address: %x\n", I2CPanic, MCPaddress[j]);
        ++I2CPanicCount;
      }

      delay(100);

      Wire.requestFrom(MCPaddress[j], 3); // Request 2 bytes of data
      // Serial.printf("Words read: %d ", Wire.available());
      //  Read 2 bytes of data  raw_adc msb, raw_adc lsb
      offset = 0;
      if (j == 1)
      {
        offset = 8; // second 4 words start at offset 8
      }

      if (Wire.available() == 3)
      {
        data[0] = Wire.read();
        data[1] = Wire.read();
        raw_sensor_data[i + offset] = (data[0] << 8) | data[1];
        // Serial.printf("Raw Sensor Value: %0x \n", raw_sensor_data[i+offset]);
        // Serial.println("");
      }
      else
      {
        raw_sensor_data[19] = 5;
      }
    }
  }
}

void loop()
{
  int i;
  int decimal;

  /* If I2C Errors Exceed 100 in 10 seconds then force a reboot to hopefully clear up */

  if (((I2CPanicCount - last_I2CPanicCount) > 25) && (WDT_Interval >= WDT_TIMEOUT))
  {
    Serial.printf("%d %d %d\n", I2CPanicCount, last_I2CPanicCount, WDT_Interval);
    while (1)
    {
    } // force a reboot
  }
  if (WDT_Interval++ > WDT_TIMEOUT)
  {
    WDT_Interval = 0;
  }

#if defined(ARDUINO_FEATHER_ESP32)
  esp_task_wdt_reset();
#endif

  I2CComm();

  ++masterCounter;

  if (masterCounter > 28800)
  { // Force a reboot every 8 hours
    while (1)
    {
    };
  }
  raw_sensor_data[12] = masterCounter;
  raw_sensor_data[16] = I2CPanicCount;

  client.loop();

  client.publish("Tank ESP", (byte *)raw_sensor_data, 42);

  Serial.printf("Well Pump Data: ");
  for (i = 0; i <= 16; ++i)
  {
    Serial.printf("%x ", raw_sensor_data[i]);
  }
  for (i = 17; i <= 19; ++i)
  {
    decimal = (short)raw_sensor_data[i];
    Serial.printf("%d ", decimal);
  }
  for (i = 20; i <= 20; ++i)
  {
    Serial.printf("%x ", raw_sensor_data[i]);
  }
  Serial.printf("\n");

  delay(500);
}