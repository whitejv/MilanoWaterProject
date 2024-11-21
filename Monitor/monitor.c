#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <json-c/json.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

int verbose = FALSE;

float TotalDailyGallons = 0;
float TotalGPM = 0;

#define MAX_CYCLE_COUNT 28000  // The cycle count rolls over at this value
#define OFFLINE_THRESHOLD 5    // Number of iterations before flagging as offline

// Structure to hold sensor data
typedef struct {
    int prev_cycle_count;
    int current_cycle_count;
    int unchanged_count;  // Tracks how many consecutive times the count has stayed the same
    int is_offline;
} SensorData;

// Function to monitor a single sensor's cycle count and detect offline status
void monitor_sensor(SensorData *sensor, int new_cycle_count) {
    sensor->prev_cycle_count = sensor->current_cycle_count;
    sensor->current_cycle_count = new_cycle_count;

    // Check if the cycle count has not changed
    if (sensor->current_cycle_count == sensor->prev_cycle_count) {
        sensor->unchanged_count++;
    } else {
        sensor->unchanged_count = 0;  // Reset the count if the cycle count changes
    }

    // Flag as offline if the count hasn't changed for the threshold number of iterations
    if (sensor->unchanged_count >= OFFLINE_THRESHOLD) {
        sensor->is_offline = -200;
        printf("Sensor went offline (unchanged for %d iterations)!\n", OFFLINE_THRESHOLD);
    } else {
        sensor->is_offline = 0;  // Sensor is online
    }

    // Rollover detection: handle normal cycle rollover behavior
    if (sensor->prev_cycle_count > sensor->current_cycle_count &&
        sensor->prev_cycle_count - sensor->current_cycle_count > MAX_CYCLE_COUNT / 2) {
        printf("Sensor experienced a cycle rollover.\n");
    }
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

SensorData sensors[4] = {0, 0, 0, 0,
                         0, 0, 0, 0,
                         0, 0, 0, 0,
                         0, 0, 0, 0};

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
   u_int32_t PumpRunCount = 0;
   int PumpCurrentSense[5];
   int PumpLedColor[5];

   log_message("Monitor: Started\n"); 

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

   if ((rc = MQTTClient_create(&client, mqtt_address, MONITOR_CLIENTID,
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

   printf("Subscribing to all monitor topics: %s\nfor client: %s using QoS: %d\n\n", "mwp/data/monitor/#", MONITOR_CLIENTID, QOS);
   log_message("Monitor: Subscribing to topic: %s for client: %s\n", "mwp/data/monitor/#", MONITOR_CLIENTID);
   MQTTClient_subscribe(client, "mwp/data/monitor/#", QOS);

   //No need to subscribe to our own message
   MQTTClient_unsubscribe(client, MONITOR_TOPICID);
   
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

   log_message("Monitor: Entering Main Loop\n") ;

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
      monitor_.monitor.Tank_Water_Height = tankMon_.tank.water_height;
      monitor_.monitor.Tank_Gallons = tankMon_.tank.tank_gallons;
      monitor_.monitor.Tank_Percent_Full = tankMon_.tank.tank_per_full;   
      monitor_.monitor.House_Pressure = houseMon_.house.pressurePSI;
      monitor_.monitor.Well3_Pressure = well3Mon_.well3.pressurePSI  ;
      monitor_.monitor.Irrigation_Pressure = irrigationMon_.irrigation.pressurePSI;
      monitor_.monitor.House_Gallons_Minute = houseMon_.house.gallonsMinute;
      monitor_.monitor.Well3_Gallons_Minute = well3Mon_.well3.gallonsMinute ;
      monitor_.monitor.Irrigation_Gallons_Minute = irrigationMon_.irrigation.gallonsMinute;
      monitor_.monitor.House_Gallons_Day = houseMon_.house.gallonsDay;
      monitor_.monitor.Well3_Gallons_Day = well3Mon_.well3.gallonsDay ;
      monitor_.monitor.Irrigation_Gallons_Day = irrigationMon_.irrigation.gallonsDay;
      monitor_.monitor.System_Temp = wellMon_.well.system_temp;
      monitor_.monitor.House_Water_Temp = houseMon_.house.temperatureF;
      monitor_.monitor.Irrigation_Pump_Temp = irrigationMon_.irrigation.temperatureF;
      monitor_.monitor.Air_Temp = tankMon_.tank.temperatureF;
      monitor_.monitor.Controller = irrigationMon_.irrigation.controller;
      monitor_.monitor.Zone = irrigationMon_.irrigation.zone;
      
      // Channel 2 Voltage Sensor 16 bit data
      //raw_voltage1_adc = well_sensor_payload[0];
      if (wellMon_.well.well_pump_1_on == 1)
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
      //raw_voltage2_adc = well_sensor_payload[1];
      //printf("voltage ch 2: %d\n", raw_voltage2_adc);
      if (wellMon_.well.well_pump_2_on== 1)
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
      //raw_voltage3_adc = well_sensor_payload[2];
      if (wellMon_.well.well_pump_3_on == 1)
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

      //raw_voltage4_adc = well_sensor_payload[3];
      //printf("voltage ch 4: %d\n", raw_voltage4_adc);
      if (wellMon_.well.irrigation_pump_on == 1)
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

//Check to see if any sensors are offline
      monitor_sensor(&sensors[0], wellMon_.well.cycle_count); 
      monitor_sensor(&sensors[1], houseMon_.house.cycleCount);
      monitor_sensor(&sensors[2], tankMon_.tank.cycleCount);
      monitor_sensor(&sensors[3], irrigationMon_.irrigation.cycleCount);
      

      monitor_.monitor.Well_1_LED_Bright = PumpCurrentSense[1] + sensors[0].is_offline;
      monitor_.monitor.Well_2_LED_Bright = PumpCurrentSense[2] + sensors[1].is_offline;
      monitor_.monitor.Well_3_LED_Bright = PumpCurrentSense[3] + sensors[2].is_offline;
      monitor_.monitor.Irrig_4_LED_Bright = PumpCurrentSense[4] + sensors[3].is_offline;

      monitor_.monitor.Well_1_LED_Color = PumpLedColor[1];
      monitor_.monitor.Well_2_LED_Color = PumpLedColor[2];
      monitor_.monitor.Well_3_LED_Color = PumpLedColor[3];
      monitor_.monitor.Irrig_4_LED_Color = PumpLedColor[4];

      
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
            MyPumpStats[j].StartGallons = tankMon_.tank.tank_gallons;
         }
         if (MyPumpStats[j].PumpOn == OFF && MyPumpStats[j].PumpLastState == ON)
         {
            MyPumpStats[j].RunTime += (SecondsFromMidnight - MyPumpStats[j].PumpOnTimeStamp);
            MyPumpStats[j].StopGallons = tankMon_.tank.tank_gallons;
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

      if (verbose) {   
         for (i=0; i<=MONITOR_LEN-1; i++) {
            printf("%s %f ", monitor_ClientData_var_name [i],monitor_.data_payload[i]);
         }
         printf("%s", ctime(&t));
      }
      pubmsg.payload = monitor_.data_payload;
      pubmsg.payloadlen = MONITOR_LEN * 4;
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      deliveredtoken = 0;
      if ((rc = MQTTClient_publishMessage(client, MONITOR_TOPICID, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
      {
         log_message("Monitor: Error == Failed to Publish Message. Return Code: %d\n", rc);
         printf("Failed to publish message, return code %d\n", rc);
         rc = EXIT_FAILURE;
      }
      json_object *root = json_object_new_object();
      for (i=0; i<=MONITOR_LEN-13; i++) {
         json_object_object_add(root, monitor_ClientData_var_name [i], json_object_new_double(monitor_.data_payload[i]));
      }
      for (i=18; i<=MONITOR_LEN-1; i++) {
         json_object_object_add(root, monitor_ClientData_var_name [i], json_object_new_int(monitor_.data_payload[i]));
      }
      const char *json_string = json_object_to_json_string(root);

      pubmsg.payload = (void *)json_string; // Make sure to cast the const pointer to void pointer
      pubmsg.payloadlen = strlen(json_string);
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      MQTTClient_publishMessage(client, MONITOR_JSONID, &pubmsg, &token);
      //printf("Waiting for publication of %s\non topic %s for client with ClientID: %s\n", json_string, MONITOR_TOPIC, MONITOR_MONID);
      MQTTClient_waitForCompletion(client, token, TIMEOUT);
      //printf("Message with delivery token %d delivered\n", token);

      json_object_put(root); // Free the memory allocated to the JSON object

      /*
       * Run at this interval
       */

      sleep(1);
   }
   log_message("Monitor: Exiting Main Loop\n") ;
   MQTTClient_unsubscribe(client, "mwp/data/monitor/#");

   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}