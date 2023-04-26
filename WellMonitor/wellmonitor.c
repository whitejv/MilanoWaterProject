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
 
   int SepticAlert = 0;

   int raw_voltage1_adc = 0;
   int raw_voltage2_adc = 0;
   int raw_voltage3_adc = 0;
   int raw_voltage4_adc = 0;
   int pump1_on = 0;
   int pump2_on = 0;
   int pump3_on = 0;
   int pump4_on = 0;

   float WaterPresSensorValue;

   int PressSwitState;

   int raw_temp = 0;
   float AmbientTempF = 0;
   float AmbientTempC = 0;

   

   log_message("WellMonitor: Started\n");

   MQTTClient client;
   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   int rc;

   if ((rc = MQTTClient_create(&client, ADDRESS, WELL_MONID,
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

   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", WELL_CLIENT, WELL_CLIENTID, QOS);
   log_message("WellMonitor: Subscribing to topic: %s for client: %s\n", WELL_CLIENT, WELL_CLIENTID);
   MQTTClient_subscribe(client, WELL_CLIENT, QOS);

   /*
    * Main Loop
    */

   log_message("WellMonitor: Entering Main Loop\n");

   while (1)
   {
      time(&t);

     
      raw_voltage1_adc = well_data_payload[0];
      if (raw_voltage1_adc > 1000) {
         pump1_on = 1 ;
      }
      else {
         pump1_on = 0;
      }

      raw_voltage2_adc = well_data_payload[1];
      if (raw_voltage2_adc > 1000) {
         pump2_on = 1 ;
      }
      else {
         pump2_on = 0;
      }     
      raw_voltage3_adc = well_data_payload[2];
      if (raw_voltage3_adc > 1000) {
         pump3_on = 1 ;
      }
      else {
         pump3_on = 0;
      }
      raw_voltage4_adc = well_data_payload[3];
      if (raw_voltage4_adc > 250) {
         pump4_on = 1 ;
      }
      else {
         pump4_on = 0;
      }

      //printf("W1: %d  W2: %d  W3: %d  W4: %d\n", raw_voltage1_adc, raw_voltage2_adc, raw_voltage3_adc, raw_voltage4_adc );
      //printf("P1: %d  P2: %d  P3: %d  P4: %d\n", pump1_on, pump2_on, pump3_on, pump4_on );
      WaterPresSensorValue = well_data_payload[4] * .00275496;
      //printf("Raw Water Pressure: 0%x  %d  %f\n", well_data_payload[4],well_data_payload[4], WaterPresSensorValue);

    

      /*
       * Convert the Discrete data
       */


      PressSwitState = well_data_payload[5];
      SepticAlert = well_data_payload[6];

      /*
       * Convert Raw Temp Sensor to degrees farenhiet
       */

      raw_temp = well_data_payload[10];
      AmbientTempC = raw_temp ;
      AmbientTempF = (AmbientTempC * 1.8) + 32.0;
      printf("Ambient Temp:%f  \n", AmbientTempF);

      /*
       * Set Firmware Version
       * firmware = well_data_payload[20] & SubFirmware;
       */

      /*
       * Load Up the Data
       */

      well_sensor_payload[0] = pump1_on;
      well_sensor_payload[1] = pump2_on;
      well_sensor_payload[2] = pump3_on;
      well_sensor_payload[3] = pump4_on;
      well_sensor_payload[4] = WaterPresSensorValue;
      well_sensor_payload[5] = PressSwitState;
      well_sensor_payload[6] = SepticAlert;
      well_sensor_payload[7] = 0;
      well_sensor_payload[8] = 0;
      well_sensor_payload[9] = 0;
      well_sensor_payload[10] = AmbientTempF;
      well_sensor_payload[11] = 0;
      well_sensor_payload[12] = well_data_payload[12];
      well_sensor_payload[13] = 0;
      well_sensor_payload[14] = raw_voltage1_adc;
      well_sensor_payload[15] = raw_voltage2_adc;
      well_sensor_payload[16] = raw_voltage3_adc;
      well_sensor_payload[17] = raw_voltage4_adc;
      well_sensor_payload[18] = 0;
      /*
       * Load Up the Payload
       */
      /*
       for (i=0; i<=WELL_DATA; i++) {
          printf("%.3f ", well_sensor_payload[i]);
       }
       printf("%s", ctime(&t));
      */
      pubmsg.payload = well_sensor_payload;
      pubmsg.payloadlen = WELL_DATA * 4;
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      deliveredtoken = 0;
      if ((rc = MQTTClient_publishMessage(client, WELL_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
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
