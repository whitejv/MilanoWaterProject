#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

/*#define  WELL_CLIENTID	 "Well Client", #define WELL_TOPIC   "Well ESP", well_esp_ , #define WELL_LEN 21
* payload 0	 A0 Raw Sensor Current Sense Well 1 16bit
* payload 1	 A1 Raw Sensor Current Sense Well 2 16bit
* payload 2	 A2 Raw Sensor Current Sense Well 3 16bit
* payload 3	 A3 Raw Sensor Current Sense Irrigation Pump 16bit
* payload 4	 A7 Sensor House Water Pressure 16bit ADC 0-5v
* payload 5	D2 House Tank Pressure Switch (0=active)
* payload 6	D3 Septic Alert (0=active)
* payload 7	 unused
* payload 8	 unused
* payload 9	 unused
* payload 10	 Raw Temp Celcius
* payload 11	 unused
* payload 12	 Cycle Counter 16bit Int
* payload 13	 spare
* payload 14	 spare
* payload 15	 spare
* payload 16	 spare
* payload 17	 spare
* payload 18	 spare
* payload 19	 spare
* payload 20	 FW Version 4 Hex 
 */
/*
 * payload 21     Last payload is Control Word From User
 */

/* CLIENTID     "Tank Monitor", #define PUB_TOPIC   "Formatted Sensor Data", formatted_sensor_, len=88
 * payload[0] =    Pressure Sensor Value
 * payload[2] =    Water Height
 * payload[3] =    Tank Gallons
 * payload[4] =    Tank Percent Full
 * payload[5] =    Current Sensor  1 Value (Well #1)
 * payload[6] =    Current Sensor  2 Value (Well #2)
 * payload[7] =    Current Sensor  3 Value (Well #3) 
 * payload[8] =    Current Sensor  4 Value (Irrigation Pump)
 * payload[9] =    Firmware Version of ESP
 * payload[10] =    I2C Fault Count
 * payload[11] =    Cycle Count
 * payload[12] =    Ambient Temperature
 * payload[13] =    Float State 1
 * payload[14] =    Float State 2
 * payload[15] =    Float State 3
 * payload[16] =    Float State 4
 * payload[17] =    Pressure Switch State
 * payload[18] =    House Water Pressure Value
 * payload[19] =     spare
 * payload[20] =     spare
 * payload[21] =     spare
 */

/* Kalman Filter Setup */
#define DT 0.1  // Time step
#define A 1     // Matrix A
#define H 1     // Matrix H
#define Q 0.001 // Process noise covariance
#define R 0.1   // Measurement noise covariance

double x = 0; // Estimated state
double P = 1; // Estimated state covariance
double z = 0; // Measurement

/* Kalman Filter Functions */
void predict()
{
   x = A * x;         // Predict new state
   P = A * P * A + Q; // Predict new state covariance
}

void update()
{
   double K = P * H / (H * P * H + R); // Kalman gain
   x = x + K * (z - H * x);            // Update state
   P = (1 - K * H) * P;                // Update state covariance
}

MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
   // printf("Message with token value %d delivery confirmed\n", dt);
   deliveredtoken = dt;
}

/* Using an include here to allow me to reuse a chunk of code that
   would not work as a library file. So treating it like an include to
   copy and paste the same code into multiple programs.
*/

#include "../mylib/msgarrvd.c"

void connlost(void *context, char *cause)
{
   printf("\nConnection lost\n");
   printf("     cause: %s\n", cause);
}

int main(int argc, char *argv[])
{

   int i;
   time_t t;
   time(&t);
   float WaterPresSensorValue;
   float PresSensorLSB = .0000625; // lsb voltage value from datasheet
   // static float PresSensorValueArray[10] = {0.,0.,0.,0.,0.,0.,0.,0.,0.,0.};
   float PresSensorValue = 0;
   // float Old_PresSensorValue = 0;
   // static int   PresIndex = 0;
   float ConstantX = .34; // Used Excel Polynomial Fitting to come up with equation
   float Constant = .0962;
   // float PresSensorAverage = 0;
   float WaterHeight = 0;
   float TankGallons = 0;
   float TankPerFull = 0;
   float Tank_Area = 0;

   int Float100State = 0;
   int Float90State = 0;
   int Float50State = 0;
   int Float25State = 0;
   int SepticAlert = 0;

   int raw_voltage1_adc = 0;
   int raw_voltage2_adc = 0;
   int raw_voltage3_adc = 0;
   int raw_voltage4_adc = 0;

   float PresSensorRawValue;
   // float Old_PresSensorRawValue;
   // unsigned short int PresSensorRawValue;
   int PressSwitState;

   int raw_temp = 0;
   float AmbientTempF = 0;
   float AmbientTempC = 0;

   // Set initial state and state covariance for Kalman filter
   x = 0;
   P = 1;

   log_message("WellMonitor: Started\n");

   MQTTClient client;
   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   int rc;

   if ((rc = MQTTClient_create(&client, ADDRESS, F_CLIENTID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      log_message("WellMonitor: Error == Failed to Create Client. Return Code: %d\n", rc);
      printf("Failed to create client, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      log_message("WellMonitor: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
      printf("Failed to set callbacks, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   conn_opts.keepAliveInterval = 20;
   conn_opts.cleansession = 1;
   // conn_opts.username = mqttUser;       //only if req'd by MQTT Server
   // conn_opts.password = mqttPassword;   //only if req'd by MQTT Server
   if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
   {
      log_message("WellMonitor: Error == Failed to Connect. Return Code: %d\n", rc);
      printf("Failed to connect, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", WELL_TOPIC, WELL_CLIENTID, QOS);
   log_message("WellMonitor: Subscribing to topic: %s for client: %s\n", WELL_TOPIC, WELL_CLIENTID);
   MQTTClient_subscribe(client, WELL_TOPIC, QOS);

   /*
    * Main Loop
    */

   log_message("WellMonitor: Entering Main Loop\n");

   while (1)
   {
      time(&t);

      /*
       * Convert Raw hydrostatic Pressure Sensor
       * A/D to Water Height, Gallons & Percent Full
       */

      PresSensorRawValue = well_data_payload[8] * PresSensorLSB;

      /*
       * Kalman Filter to smooth the hydrostatic sensor readings
       */

      z = PresSensorRawValue;
      predict();
      update();
      // printf("x: %f\n", x);  // Print updated state
      PresSensorValue = x;
      /*
       * Rolling Average to Smooth data
       */
      /*
      PresSensorAverage = 0;
      PresSensorValue = 0;
      PresSensorValueArray[PresIndex++] = PresSensorRawValue;        //Convert sensor value to voltage
      PresIndex = PresIndex % 10;

      for( i=0; i<=9; ++i){
         PresSensorAverage += PresSensorValueArray[i];
      }
      PresSensorValue = PresSensorAverage/10;
      //printf("Pressure Sensor Raw: %f delta: %f Smoothed: %f delta: %f\n",  PresSensorRawValue, PresSensorRawValue-Old_PresSensorRawValue, PresSensorValue, PresSensorValue-Old_PresSensorValue);
      //Old_PresSensorRawValue = PresSensorRawValue;
      //Old_PresSensorValue = PresSensorValue;
      */
      /*
       *** Use the Equation y=Constandx(x) + Constant solve for x to compute Water Height in tank
       */

      if (PresSensorValue <= Constant)
      {
         PresSensorValue = Constant;
      }

      WaterHeight = ((PresSensorValue - ConstantX) / Constant) + .1; // The .1 accounts for the sensor not sitting on the bottom
      // printf("Water Height = %f\n", WaterHeight);

      /*
       * Use the Equation (PI*R^2*WaterHeight)*VoltoGal to compute Water Gallons in tank
       */
      Tank_Area = PI * Tank_Radius_sqd; // area of base of tank
      TankGallons = ((Tank_Area)*WaterHeight) * VoltoGal;
      // printf("Gallons in Tank = %f\n", TankGallons);

      /*
       *  Use the Equation Calculated Gallons/Max Gallons to compute Percent Gallons in tank
       */

      TankPerFull = TankGallons / MaxTankGal * 100;
      // printf("Percent Gallons in Tank = %f\n", TankPerFull);

      // Channel 2 Voltage Sensor 16 bit data
      raw_voltage1_adc = (int16_t)well_data_payload[1];

      // Channel 3 Voltage Sensor 16 bit data
      raw_voltage2_adc = (int16_t)well_data_payload[2];
      // printf("voltage ch 2: %d\n", raw_voltage2_adc);

      // Channel 4 Voltage Sensor 16 bit data
      raw_voltage3_adc = (int16_t)well_data_payload[3];

      // MCP3428 #2 Channel 4 Voltage Sensor 16 bit data
      WaterPresSensorValue = (int16_t)well_data_payload[9] * .00275496;
      // printf("Raw Water Pressure: 0%x  %d  %f\n", well_data_payload[9],well_data_payload[9], WaterPresSensorValue);

      raw_voltage4_adc = (int16_t)well_data_payload[11];

      /*
       * Convert the Discrete data
       */

      Float100State = (well_data_payload[4] & 0x0001);
      Float90State = (well_data_payload[4] & 0x0002) >> 1;
      Float50State = (well_data_payload[4] & 0x0004) >> 2;
      Float25State = (well_data_payload[4] & 0x0008) >> 3;
      PressSwitState = (well_data_payload[4] & 0x0010) >> 4;
      SepticAlert = (well_data_payload[4] & 0x0020) >> 5;

      /*
       * Convert Raw Temp Sensor to degrees farenhiet
       */

      raw_temp = well_data_payload[5];
      AmbientTempC = raw_temp * .0625; // LSB weight for 12 bit conversion is .0625
      AmbientTempF = (AmbientTempC * 1.8) + 32.0;
      // printf("Ambient Temp:%f  \n", AmbientTempF);

      /*
       * Set Firmware Version
       * firmware = well_data_payload[20] & SubFirmware;
       */

      /*
       * Load Up the Data
       */

      formatted_sensor_payload[0] = PresSensorValue;
      formatted_sensor_payload[1] = WaterHeight;
      formatted_sensor_payload[2] = TankGallons;
      formatted_sensor_payload[3] = TankPerFull;
      formatted_sensor_payload[4] = raw_voltage1_adc;
      formatted_sensor_payload[5] = raw_voltage2_adc;
      formatted_sensor_payload[6] = raw_voltage3_adc;
      formatted_sensor_payload[7] = raw_voltage4_adc;
      formatted_sensor_payload[8] = firmware;
      formatted_sensor_payload[9] = well_data_payload[16];  // I2C Faults
      formatted_sensor_payload[10] = well_data_payload[12]; // Cycle Count
      formatted_sensor_payload[11] = AmbientTempF;
      formatted_sensor_payload[12] = Float100State;
      formatted_sensor_payload[13] = Float90State;
      formatted_sensor_payload[14] = Float50State;
      formatted_sensor_payload[15] = Float25State;
      formatted_sensor_payload[16] = PressSwitState;
      formatted_sensor_payload[17] = WaterPresSensorValue;
      formatted_sensor_payload[18] = SepticAlert;
      /*
       * Load Up the Payload
       */
       for (i=0; i<=F_LEN; i++) {
          printf("%.3f ", formatted_sensor_payload[i]);
       }
       printf("%s", ctime(&t));

      pubmsg.payload = formatted_sensor_payload;
      pubmsg.payloadlen = F_LEN * 4;
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      deliveredtoken = 0;
      if ((rc = MQTTClient_publishMessage(client, F_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
      {
         log_message("WellMonitor: Error == Failed to Publish Message. Return Code: %d\n", rc);
         printf("Failed to publish message, return code %d\n", rc);
         rc = EXIT_FAILURE;
      }

      /*
       * Run at this interval
       */

      sleep(1);
   }
   log_message("WellMonitor: Exiting Main Loop\n") ;
   MQTTClient_unsubscribe(client, WELL_TOPIC);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
