#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

int verbose = FALSE;

/*payload[0] =    Pressure Sensor Value
 * payload[1] =    Water Height
 * payload[2] =    Tank Gallons
 * payload[3] =    Tank Percent Full
 * payload[4] =    Current Sensor  1 Value
 * payload[5] =    Current Sensor  2 Value
 * payload[6] =    Current Sensor  3 Value
 * payload[7] =    Current Sensor  4 Value
 * payload[8] =    Firmware Version of ESP
 * payload[9] =    I2C Fault Count
 * payload[10] =    Cycle Count
 * payload[11] =    Ambient Temperature
 * payload[12] =    Float State 1
 * payload[13] =    Float State 2
 * payload[14] =    Float State 3
 * payload[15] =    Float State 4
 * payload[16] =    Pressure Switch State
 * payload[17] =    House Water Pressure Value
 * payload[18] =    Septic Alert
 * payload[19] =     spare
 * payload[20] =     spare
 */

/* payload[0] =     PumpCurrentSense[1];
 * payload[1] =     PumpCurrentSense[2];
 * payload[2] =     PumpCurrentSense[3];
 * payload[3] =     PumpCurrentSense[4];
 * payload[4] =     PumpLedColor[1];
 * payload[5] =     PumpLedColor[2];
 * payload[6] =     PumpLedColor[3];
 * payload[7] =     PumpLedColor[4];
 * payload[8] =     PumpRunCount;  //byte4-pump4;byte3-pump3;byte2-pump2;byte1-pump1
 * payload[9] =    PumpRunTime{1] ; //Seconds
 * payload[10] =    PumpRunTime{2] ; //Seconds
 * payload[11] =    PumpRunTime{3] ; //Seconds
 * payload[12] =    PumpRunTime{4] ; //Seconds
 * payload[13] =     43floatState;  //byte34-float4;byte123-float3
 * payload[14] =     21floatState;  //bytes34-float2;byte12-float1
 * payload[15] =     AllfloatLedcolor;  //byte4-color4;byte3-color3;byte2-color2;byte1-color1
 * payload[16] =    Septic Relay Alert
 * payload[17] =    Septic Relay Alert Color
 * payload[18] =    Pressure Relay Sense
 * payload[19] =    Pressure LED Color
 * payload[20] =    spare
 */

float TotalDailyGallons = 0;
float TotalGPM = 0;

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
   FILE *fptr;
   time_t t;
   struct tm timenow;
   time(&t);
   int SecondsFromMidnight = 0;
   int PriorSecondsFromMidnight = 0;
   u_int32_t A43floatState = 0;
   u_int32_t A21floatState = 0;
   u_int32_t AllfloatLedcolor = 0;
   int Float100State = 0;
   int Float90State = 0;
   int Float50State = 0;
   int Float25State = 0;
   int PressSwitState = 0;
   int raw_voltage1_adc = 0;
   int raw_voltage2_adc = 0;
   int raw_voltage3_adc = 0;
   int raw_voltage4_adc = 0;
   int pressState = 0;
   int pressLedColor;
   int floatstate[5];
   int floatLedcolor[5];
   u_int32_t PumpRunCount = 0;
   int PumpCurrentSense[5];
   int PumpLedColor[5];
   int SepticAlert = 0;
   int SepticAlertColor;
   int SepticAlertState;

   log_message("TankMonitor: Started\n"); 

   MQTTClient client;
   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   int rc;
   int opt;
   const char *mqtt_ip;
   int mqtt_port;

   while ((opt = getopt(argc, argv, "vPD")) != -1) {
      switch (opt) {
         case 'v':
               verbose = TRUE;
               break;
         case 'P':
               mqtt_ip = PROD_MQTT_IP;
               mqtt_port = PROD_MQTT_PORT;
               break;
         case 'D':
               mqtt_ip = DEV_MQTT_IP;
               mqtt_port = DEV_MQTT_PORT;
               break;
         default:
               fprintf(stderr, "Usage: %s [-v] [-P | -D]\n", argv[0]);
               return 1;
      }
   }

   if (verbose) {
      printf("Verbose mode enabled\n");
   }

   if (mqtt_ip == NULL) {
      fprintf(stderr, "Please specify either Production (-P) or Development (-D) server\n");
      return 1;
   }

   char mqtt_address[256];
   snprintf(mqtt_address, sizeof(mqtt_address), "tcp://%s:%d", mqtt_ip, mqtt_port);

   printf("MQTT Address: %s\n", mqtt_address);

   if ((rc = MQTTClient_create(&client, mqtt_address, MON_ID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      log_message("TankMonitor: Error == Failed to Create Client. Return Code: %d\n", rc);
      printf("Failed to create client, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      log_message("TankMonitor: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
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
      log_message("TankMonitor: Error == Failed to Connect. Return Code: %d\n", rc);
      printf("Failed to connect, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", TANK_TOPIC, MON_ID, QOS);
   log_message("TankMonitor: Subscribing to topic: %s for client: %s\n", TANK_TOPIC, MON_ID);
   MQTTClient_subscribe(client, TANK_TOPIC, QOS);
   
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", WELL_TOPIC, MON_ID, QOS);
   log_message("TankMonitor: Subscribing to topic: %s for client: %s\n", WELL_TOPIC, MON_ID);
   MQTTClient_subscribe(client, WELL_TOPIC, QOS);

   /*
    * Initialize the data file with headers
    */
   fptr = fopen(pumpdata, "a");
   fprintf(fptr, "Well #, Start Gallons, Stop Gallons, Run Time (sec) \n");
   fclose(fptr);
   fptr = fopen(datafile, "a");
   fprintf(fptr, "Pump #, Run Count, Run Time (sec) \n");
   fclose(fptr);

   /*
    * Main Loop
    */

   log_message("TankMonitor: Entering Main Loop\n") ;

   while (1)
   {
      time(&t);
      localtime_r(&t, &timenow);

      /*
       * Check the time and see if we passed midnight
       * if we have then reset data like MyPumpStats to 0 for a new day
       */

      SecondsFromMidnight = (timenow.tm_hour * 60 * 60) + (timenow.tm_min * 60) + timenow.tm_sec;
      if (SecondsFromMidnight < PriorSecondsFromMidnight)
      {
         fptr = fopen(datafile, "a");

         for (j = 1; j <= NPumps; j++)
         {
            fprintf(fptr, "%d, %d, %d ", j, MyPumpStats[j].RunCount, MyPumpStats[j].RunTime);
            MyPumpStats[j].PumpOn = 0;
            MyPumpStats[j].PumpOnTimeStamp = 0;
            MyPumpStats[j].PumpLastState = 0;
            MyPumpStats[j].RunCount = 0;
            MyPumpStats[j].RunTime = 0;
         }
         fprintf(fptr, "%s", ctime(&t));
         fclose(fptr);
      }
      // printf("seconds since midnight: %d\n", SecondsFromMidnight);
      PriorSecondsFromMidnight = SecondsFromMidnight;

      /*
       * Compute Monitor Values Based on Inputs from
       * Sensor Data and Format for easy use with Blynk
       */

      // Channel 2 Voltage Sensor 16 bit data
      raw_voltage1_adc = well_sensor_payload[0];
      if (raw_voltage1_adc == 1)
      {
         PumpCurrentSense[1] = 255;
         Pump[1].PumpPower = ON;
         PumpLedColor[1] = BLUE;
      }
      else
      {
         PumpCurrentSense[1] = 255;
         Pump[1].PumpPower = OFF;
         PumpLedColor[1] = GREEN;
      }

      // Channel 3 Voltage Sensor 16 bit data
      raw_voltage2_adc = well_sensor_payload[1];
      //printf("voltage ch 2: %d\n", raw_voltage2_adc);
      if (raw_voltage2_adc == 1)
      {
         PumpCurrentSense[2] = 255;
         Pump[2].PumpPower = ON;
         PumpLedColor[2] = BLUE;
      }
      else
      {
         PumpCurrentSense[2] = 255;
         Pump[2].PumpPower = OFF;
         PumpLedColor[2] = GREEN;
      }

      // Channel 4 Voltage Sensor 16 bit data
      raw_voltage3_adc = well_sensor_payload[2];
      if (raw_voltage3_adc == 1)
      {
         PumpCurrentSense[3] = 255;
         Pump[3].PumpPower = ON;
         PumpLedColor[3] = BLUE;
      }
      else
      {
         PumpCurrentSense[3] = 255;
         Pump[3].PumpPower = OFF;
         PumpLedColor[3] = GREEN;
      }

      // MCP3428 #2 Channel 4 Voltage Sensor 16 bit data

      raw_voltage4_adc = well_sensor_payload[3];
      //printf("voltage ch 4: %d\n", raw_voltage4_adc);
      if (raw_voltage4_adc == 1)
      {
         PumpCurrentSense[4] = 255;
         Pump[4].PumpPower = ON;
         PumpLedColor[4] = BLUE;
      }
      else
      {
         PumpCurrentSense[4] = 255;
         Pump[4].PumpPower = OFF;
         PumpLedColor[4] = GREEN;
      }

      /*
       * Convert the Discrete data
       */

      Float100State = tank_sensor_payload[12];
      Float90State = tank_sensor_payload[13];
      Float50State = tank_sensor_payload[14];
      Float25State = tank_sensor_payload[15];
      PressSwitState = well_sensor_payload[16];
      SepticAlert = well_sensor_payload[18];

      if (Float100State == 1)
      {
         floatstate[1] = 255;
         floatLedcolor[1] = GREEN;
      }
      else
      {
         floatstate[1] = 255;
         floatLedcolor[1] = RED;
      }

      if (Float90State == 1)
      {
         floatstate[2] = 255;
         floatLedcolor[2] = GREEN;
      }
      else
      {
         floatstate[2] = 255;
         floatLedcolor[2] = RED;
      }

      if (Float50State == 1)
      {
         floatstate[3] = 255;
         floatLedcolor[3] = GREEN;
      }
      else
      {
         floatstate[3] = 255;
         floatLedcolor[3] = RED;
      }

      if (Float25State == 1)
      {
         floatstate[4] = 255;
         floatLedcolor[4] = GREEN;
      }
      else
      {
         floatstate[4] = 255;
         floatLedcolor[4] = RED;
      }

      if (PressSwitState == 1)
      {
         pressState = 255;
         pressLedColor = BLUE;
      }
      else
      {
         pressState = 255;
         pressLedColor = GREEN;
      }

      if (SepticAlert == 1)
      {
         SepticAlertState = 255;
         SepticAlertColor = RED;
      }
      else
      {
         SepticAlertState = 255;
         SepticAlertColor = GREEN;
      }

      /*
       * Compute Pump Stats for each pump
       */
      for (j = 1; j <= NPumps; j++)
      {
         MyPumpStats[j].PumpOn = Pump[j].PumpPower;
         if (MyPumpStats[j].PumpOn == ON)
         {
            // MyPumpStats[j].PumpOnTimeStamp = SecondsFromMidnight ;
         }
         if (MyPumpStats[j].PumpOn == ON && MyPumpStats[j].PumpLastState == OFF)
         {
            MyPumpStats[j].PumpOnTimeStamp = SecondsFromMidnight;
            MyPumpStats[j].StartGallons = tank_sensor_payload[2];
         }
         if (MyPumpStats[j].PumpOn == OFF && MyPumpStats[j].PumpLastState == ON)
         {
            MyPumpStats[j].RunTime += (SecondsFromMidnight - MyPumpStats[j].PumpOnTimeStamp);
            MyPumpStats[j].StopGallons = tank_sensor_payload[2];
            ++MyPumpStats[j].RunCount;
            /* Write Individual record for Well #3 to monitor GPM*/
            if (j == 3 || j == 4)
            {
               fptr = fopen(pumpdata, "a");
               fprintf(fptr, "%d, %d, %d, %d ", j,
                       MyPumpStats[j].StartGallons,
                       MyPumpStats[j].StopGallons,
                       (SecondsFromMidnight - MyPumpStats[j].PumpOnTimeStamp));
               fprintf(fptr, "%s", ctime(&t));
               fclose(fptr);
            }
         }
         MyPumpStats[j].PumpLastState = MyPumpStats[j].PumpOn;

         /*
          printf("MyPumpStats[%0d]  On: %d   TimeStamp: %d   LastState: %d   Count:  %d   Time: %d \n", j, \
          MyPumpStats[j].PumpOn, \
          MyPumpStats[j].PumpOnTimeStamp, \
          MyPumpStats[j].PumpLastState, \
          MyPumpStats[j].RunCount, \
          MyPumpStats[j].RunTime) ;
          */
      }
      /*
       * Set Firmware Version
       * firmware = formatted_sensor_payload[20] & SubFirmware;
       */
      /*
       * Bit Pack Some Data
       */
      A43floatState = 0;
      A21floatState = 0;
      A43floatState = floatstate[4] << 16 | floatstate[3];
      A21floatState = floatstate[2] << 16 | floatstate[1];

      AllfloatLedcolor = 0;
      AllfloatLedcolor = floatLedcolor[4] << 24 |
                         floatLedcolor[3] << 16 |
                         floatLedcolor[2] << 8 |
                         floatLedcolor[1];
      PumpRunCount = 0;
      PumpRunCount = MyPumpStats[4].RunCount << 24 |
                     MyPumpStats[3].RunCount << 16 |
                     MyPumpStats[2].RunCount << 8 |
                     MyPumpStats[1].RunCount;
      /*
       * Load Up the Data
       */

      /* CLIENTID     "Tank Subscriber", TOPIC "Monitor Data", monitor_sensor_ */
      monitor_payload[0] = PumpCurrentSense[1];
      monitor_payload[1] = PumpCurrentSense[2];
      monitor_payload[2] = PumpCurrentSense[3];
      monitor_payload[3] = PumpCurrentSense[4];
      monitor_payload[4] = PumpLedColor[1];
      monitor_payload[5] = PumpLedColor[2];
      monitor_payload[6] = PumpLedColor[3];
      monitor_payload[7] = PumpLedColor[4];
      monitor_payload[8] = PumpRunCount;
      monitor_payload[9] = MyPumpStats[1].RunTime;
      monitor_payload[10] = MyPumpStats[2].RunTime;
      monitor_payload[11] = MyPumpStats[3].RunTime;
      monitor_payload[12] = MyPumpStats[4].RunTime;
      monitor_payload[13] = A43floatState;
      monitor_payload[14] = A21floatState;
      monitor_payload[15] = AllfloatLedcolor;
      monitor_payload[16] = SepticAlertState;
      monitor_payload[17] = SepticAlertColor;
      monitor_payload[18] = pressState;
      monitor_payload[19] = pressLedColor;
      
      if (verbose) {
         for (i = 0; i <= M_LEN; i++)
         {
            printf("%0x ", monitor_payload[i]);
         }
         printf("%s", ctime(&t));
      }
      pubmsg.payload = monitor_payload;
      pubmsg.payloadlen = M_LEN * 4;
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      deliveredtoken = 0;
      if ((rc = MQTTClient_publishMessage(client, M_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
      {
         log_message("TankMonitor: Error == Failed to Publish Message. Return Code: %d\n", rc);
         printf("Failed to publish message, return code %d\n", rc);
         rc = EXIT_FAILURE;
      }

      /*
       * Run at this interval
       */

      sleep(1);
   }
   log_message("TankMonitor: Exiting Main Loop\n") ;
   MQTTClient_unsubscribe(client, FLOW_TOPIC);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
