#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

/* External Variables
SensorData = {
"hydro_stat_pressure",
"water_height",
"tank_gallons",
"tank_per_full",
"well_pump_1",
"well_pump_2",
"well_pump_3",
"irrigation_pump",
"fw_version",
"i2c_fault_count",
"cycle_count",
"ambient_temp",
"float_state_1",
"float_state_2",
"float_state_3",
"float_state_4",
"pressure_tank_switch",
"house_water_pressure",
"septic_alert",
"spare_1",
"spare_2"
};
AlertData = {
"pump_no_start",
"spare1",
"spare2",
"spare3",
"spare4",
"spare5",
"spare6",
"spare7",
"spare8",
"spare9",
"spare10",
"spare11",
"spare12",
"spare13",
"spare14",
"spare15",
"spare16",
"spare17",
"spare18",
"spare19",
"spare20"
};
*/

enum AlarmState
{
   inactive = 0,
   trigger = 1,
   active = 2,
   reset = 3,
   timeout = 4
};

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
   int i = 0;
   int j = 0;

   struct FormattedSensorData SensorData;
   struct AlertSensorData AlertData;

   static enum AlarmState Alarms[20] = {0};
   static int TimeOuts[20] = {0};
   static int Timers[20] = {0};

   int Pump1State;
   int Pump2State;
   int Pump3State;
   int Pump4State;
   
   time_t t;
   time(&t);

   log_message("Alert: Started\n");

   MQTTClient client;
   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   int rc;

   if ((rc = MQTTClient_create(&client, ADDRESS, A_CLIENTID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      log_message("Alert: Error == Failed to Create Client. Return Code: %d\n", rc);
      printf("Failed to create client, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      log_message("Alert: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
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
      log_message("Alert: Error == Failed to Connect. Return Code: %d\n", rc);
      printf("Failed to connect, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", F_TOPIC, A_CLIENTID, QOS);
   log_message("Alert: Subscribing to topic: %s for client: %s\n", F_TOPIC, A_CLIENTID);
   MQTTClient_subscribe(client, F_TOPIC, QOS);

   /*
    * Main Loop
    */

   log_message("Alert: Entering Main Loop\n");

   while (1)
   {


      /*
       *  Populate the structure with the sensor array data
       */

      memcpy((void *)&SensorData, (void *)formatted_sensor_payload, sizeof(struct FormattedSensorData));

      if (SensorData.well_pump_1 > 2500)
      {
         Pump1State = ON;
      }
      else
      {
         Pump1State = OFF;
      }
      if (SensorData.well_pump_2 > 2500)
      {
         Pump2State = ON;
      }
      else
      {
         Pump2State = OFF;
      }
      if (SensorData.well_pump_3 > 2500)
      {
         Pump3State = ON;
      }
      else
      {
         Pump3State = OFF;
      }
      if (SensorData.irrigation_pump > 2500)
      {
         Pump4State = ON;
      }
      else
      {
         Pump4State = OFF;
      }

      /**	1 -	Critical	Tank Critically Low	*/

      /**	2- 	Critical	Irrigation Pump Temp Low/High	*/

      /**	3 -	Critical	House Water Pressure Low	*/

      /**	4 -	Critical	Septic System Alert	*/

      /**	5 - 	Critical	Irrigation Pump Run Away	*/

      /**	6 -	Critical	Well 3 Pump Run Away	*/

      /**	7 -	Warn	Well Pumps Not Starting	*/

      switch (Alarms[7])
      {
      case (timeout):
         if (TimeOuts[7] == 0)
         {
            Alarms[7] = inactive;
         }
         else
         {
            TimeOuts[7]--;
         }
         break;
      case (inactive):
         if (SensorData.pressure_tank_switch == ON)
         {
            Alarms[7] = trigger;
            Timers[7] = 0;
         }
         break;
      case (trigger):
         if (Timers[7] >= 10)
         {
            if ((Pump1State == OFF || Pump2State == OFF) && SensorData.pressure_tank_switch == ON)
            {
               Alarms[7] = active;
               Timers[7] = 0;
            }
         }
         else
         {
            Timers[7]++;
         }
         break;
      case (active):
         if ((Pump1State == OFF || Pump2State == OFF) && SensorData.pressure_tank_switch == ON)
         {
            AlertData.pump_no_start = 1;
         }
         else
         {
            Alarms[7] = reset;
         }
         break;
      case (reset):
         AlertData.pump_no_start = 0;
         Timers[7] = 0;
         TimeOuts[7] = 60;
         Alarms[7] = timeout;
         break;
      default:
         break;
      }

      /**	8 - 	Warn	Well Pumps Runtime Exceeded	*/

      /**	9  -	Warn	Well Pumps Cycles Excessive	*/

      /**	10 -	Info	Well Protect Circuit Active	*/

      /**	11 - 	Warn	Tank Overfill Condition	*/

      /*
       * Load Up the Data
       */

      AlertData.spare1 = 0;
      AlertData.spare2 = 0;
      AlertData.spare3 = 0;
      AlertData.spare4 = 0;
      AlertData.spare5 = 0;
      AlertData.spare6 = 0;
      AlertData.spare7 = 0;
      AlertData.spare8 = 0;
      AlertData.spare9 = 0;
      AlertData.spare10 = 0;
      AlertData.spare11 = 0;
      AlertData.spare12 = 0;
      AlertData.spare13 = 0;
      AlertData.spare14 = 0;
      AlertData.spare15 = 0;
      AlertData.spare16 = 0;
      AlertData.spare17 = 0;
      AlertData.spare18 = 0;
      AlertData.spare19 = 0;
      AlertData.spare20 = 0;

      /*
       * Publish the Data
       */
      memcpy((void *)&alert_sensor_payload, (void *)&AlertData, sizeof(struct AlertSensorData));

      for (i = 0; i < A_LEN; i++)
      {
         printf("%d ", alert_sensor_payload[i]);
      }
      printf("%s", ctime(&t));

      pubmsg.payload = alert_sensor_payload;
      pubmsg.payloadlen = A_LEN * 4;
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      deliveredtoken = 0;
      if ((rc = MQTTClient_publishMessage(client, A_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
      {
         log_message("Alert: Error == Failed to Publish Message. Return Code: %d\n", rc);
         printf("Failed to publish message, return code %d\n", rc);
         rc = EXIT_FAILURE;
      }

      /*
       * Run at this interval
       */

      sleep(1);
   }
   log_message("Alert: Exiting Main Loop\n");
   MQTTClient_unsubscribe(client, F_TOPIC);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
