#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <json-c/json.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

int verbose = FALSE;

float TotalDailyGallons = 0;
float TotalGPM = 0;


int find_active_station(Controller* controller) {
    for (int i = 0; i < MAX_ZONES; i++) {
        if (controller->states[i].status == 1) {
            return controller->states[i].station;
        }
    }
    return -1;  // return -1 if no active station is found
}

MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
   //printf("Message with token value %d delivery confirmed\n", dt);
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

int main(int argc, char* argv[])
{
   int i=0;
   FILE *fptr;
   time_t t;
   time_t start_t, end_t;
   double diff_t;
   struct tm timenow;
   time(&t);
   int SecondsFromMidnight = 0 ;
   int PriorSecondsFromMidnight =0;
   float irrigationPressure = 0;
   float temperatureF;

   float dailyGallons = 0;
   float avgflowRateGPM = 0;
    
   int pumpState = 0;
   int lastpumpState = 0;
   int startGallons = 0;
   int stopGallons = 0;
   int tankstartGallons = 0;
   int tankstopGallons = 0;

   #define RAINBIRD_COMMAND_DELAY 20
   #define RAINBIRD_REQUEST_DELAY 20
   int rainbirdRequest = FALSE;
   int rainbirdRecvRequest = FALSE;
   int rainbirdDelay = RAINBIRD_COMMAND_DELAY ; //wait 10 seconds to query results
   int rainbirdReqDelay = RAINBIRD_REQUEST_DELAY  ; //wait 20 seconds to query results

   
   MQTTClient client;
   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   int rc;
   int opt;
   const char *mqtt_ip;
   int mqtt_port;
   int training_mode = FALSE;
   char training_filename[256];
   while ((opt = getopt(argc, argv, "vPDT:")) != -1) {
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
         case 'T':
            {
                training_mode = TRUE;
                
                if (optarg != NULL && strlen(optarg) > 0) {
                    snprintf(training_filename, 256, "%s%s", trainingdata, optarg);
                    // Use training_mode and training_filename as needed
                } else {
                    fprintf(stderr, "Error: No filename provided for the -T option.\n");
                    fprintf(stderr, "Usage: %s [-v] [-P | -D] [-T filename]\n", argv[0]);
                    return 1;
                }
            }
            break;
         default:
               fprintf(stderr, "Usage: %s [-v] [-P | -D] [-T filename]\n", argv[0]);
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

   log_message("IrrigationMonitor Started\n");

   if ((rc = MQTTClient_create(&client, mqtt_address, IRRIGATIONMON_CLIENTID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to create client, return code %d\n", rc);
      log_message("IrrigationMonitor Error == Failed to Create Client. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to set callbacks, return code %d\n", rc);
      log_message("IrrigationMonitor Error == Failed to Set Callbacks. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   conn_opts.keepAliveInterval = 20;
   conn_opts.cleansession = 1;
   //conn_opts.username = mqttUser;       //only if req'd by MQTT Server
   //conn_opts.password = mqttPassword;   //only if req'd by MQTT Server
   if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to connect, return code %d\n", rc);
      log_message("IrrigationMonitor Error == Failed to Connect. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", IRRIGATIONSENS_CLIENTID, IRRIGATIONSENS_TOPICID, QOS);
   log_message("IrrigationMonitor Subscribing to topic: %s for client: %s\n", IRRIGATIONSENS_CLIENTID, IRRIGATIONSENS_TOPICID);
   MQTTClient_subscribe(client, IRRIGATIONSENS_TOPICID, QOS);
   
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", WELLMON_TOPICID, WELLMON_CLIENTID, QOS);
   log_message("IrrigationMonitor Subscribing to topic: %s for client: %s\n", WELLMON_TOPICID, WELLMON_CLIENTID);
   MQTTClient_subscribe(client, WELLMON_TOPICID, QOS);

   printf("Subscribing to topic: %s for client: %s using QoS: 0\n", RAINBIRDRESPONSE_TOPICID, RAINBIRDRESPONSE_CLIENTID );
   log_message("IrrigationMonitor Subscribing to topic: %s for client: %s\n",RAINBIRDRESPONSE_TOPICID, RAINBIRDRESPONSE_CLIENTID);
   MQTTClient_subscribe(client, RAINBIRDRESPONSE_TOPICID, 0);
   
   /*
    * Main Loop
    */

   log_message("IrrigationMonitor Entering Main Loop\n") ;
   
   while(1) {
      time(&t);
      localtime_r(&t, &timenow);
      
      /*
       * Check the time and see if we passed midnight
       * if we have then reset data like MyPumpStats to 0 for a new day
       */
      
      SecondsFromMidnight = (timenow.tm_hour * 60 * 60) + (timenow.tm_min * 60) + timenow.tm_sec ;
      if (SecondsFromMidnight < PriorSecondsFromMidnight) {
         
         fptr = fopen(flowfile, "a");
         
         /* reset 24 hr stuff */
         
         fprintf(fptr, "Daily Irrigation Gallons Used: %f %s", dailyGallons, ctime(&t));
         dailyGallons = 0; 
         fclose(fptr);
         
      }
      //printf("seconds since midnight: %d\n", SecondsFromMidnight);
      PriorSecondsFromMidnight = SecondsFromMidnight ;
      
      /*
      * Call the flow monitor function
      */
      flowmon(irrigationSens_.irrigation.new_data_flag, irrigationSens_.irrigation.milliseconds, irrigationSens_.irrigation.pulse_count, &avgflowRateGPM, &dailyGallons, .935) ;

      irrigationPressure = (irrigationSens_.irrigation.adc_sensor * .1336) - 3.523 ;
      
      memcpy(&temperatureF, &irrigationSens_.irrigation.temp_w1, sizeof(float));
        
      /*
       * If irrigation pump is running try and determine what Controller & Zone is active
       */
      if (rainbirdRecvRequest == TRUE) {
         if (rainbirdDelay == 0) {
            int active_station = find_active_station(&Front_Controller);
            if (active_station != -1) {
               printf("Active station for Front_Controller is: %d\n", active_station);
               irrigationMon_.irrigation.FrontControllerActive = 1;
               irrigationMon_.irrigation.FrontActiveZone       = active_station;
            } else {
               printf("No active station for Front_Controller\n");
               irrigationMon_.irrigation.FrontControllerActive = 0;
               irrigationMon_.irrigation.FrontActiveZone       = 0;
            }

            active_station = find_active_station(&Back_Controller);
            if (active_station != -1) {
               printf("Active station for Back_Controller is: %d\n", active_station);
               irrigationMon_.irrigation.BackControllerActive = 1;
               irrigationMon_.irrigation.BackActiveZone       = active_station;
            } else {
               printf("No active station for Back_Controller\n");
               irrigationMon_.irrigation.BackControllerActive = 0; 
               irrigationMon_.irrigation.BackActiveZone       = 0;
            }

            rainbirdRecvRequest = FALSE;
            rainbirdDelay = RAINBIRD_COMMAND_DELAY;
         } else {
            rainbirdDelay--;
         }
      }
      /*
       * Load Up the Data
       */
      
      irrigationMon_.irrigation.FlowPerMin = avgflowRateGPM;
      irrigationMon_.irrigation.TotalFlow =   dailyGallons;
      irrigationMon_.irrigation.Pressure  =  irrigationPressure;
      irrigationMon_.irrigation.PumpTemp  =  temperatureF;
      irrigationMon_.irrigation.cycle_count =  irrigationSens_.irrigation.cycle_count ;
      irrigationMon_.irrigation.fw_version = 0;

      
      if (verbose) {
         for (i=0; i<=IRRIGATIONMON_LEN-1; i++) {
            printf("%.3f ", irrigationMon_.data_payload[i]);
         }
         printf("%s", ctime(&t));
      }
      
      pubmsg.payload = irrigationMon_.data_payload;
      pubmsg.payloadlen = IRRIGATIONMON_LEN * 4;
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      deliveredtoken = 0;
      if ((rc = MQTTClient_publishMessage(client, IRRIGATIONMON_TOPICID, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
      {
         printf("Failed to publish message, return code %d\n", rc);
         log_message("IrrigationMonitor Error == Failed to Publish Message. Return Code: %d\n", rc);
         rc = EXIT_FAILURE;
      }
      json_object *root = json_object_new_object();
      for (i=0; i<=WELLMON_LEN-1; i++) {
         json_object_object_add(root, irrigationmon_ClientData_var_name [i], json_object_new_double(irrigationMon_.data_payload[i]));
      }

      const char *json_string = json_object_to_json_string(root);

      pubmsg.payload = (void *)json_string; // Make sure to cast the const pointer to void pointer
      pubmsg.payloadlen = strlen(json_string);
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      MQTTClient_publishMessage(client, IRRIGATIONMON_JSONID, &pubmsg, &token);
      //printf("Waiting for publication of %s\non topic %s for client with ClientID: %s\n", json_string, TANK_TOPIC, TANK_MONID);
      MQTTClient_waitForCompletion(client, token, TIMEOUT);
      //printf("Message with delivery token %d delivered\n", token);

      json_object_put(root); // Free the memory allocated to the JSON object
      /*
       * Run at this interval
       */
      /*
       * Log the Data Based on Pump 4 on/off
       */
      if (wellMon_.well.irrigation_pump_on == 1) {
         pumpState = ON;
      }
      else {
         pumpState = OFF;
      }
      
      if ((pumpState == ON) && (lastpumpState == OFF)){
         startGallons = dailyGallons;
         time(&start_t);
         lastpumpState = ON;
         /*
          * Command rainbird to send data
          */
         rainbirdRequest = TRUE;
      }
      else if ((pumpState == OFF) && (lastpumpState == ON)){
         fptr = fopen(flowdata, "a");
         stopGallons = dailyGallons - startGallons ;
         time(&end_t);
         diff_t = difftime(end_t, start_t);
         if (irrigationMon_.irrigation.FrontControllerActive == 1) {
            fprintf(fptr, "Controller: Front ");
            fprintf(fptr, "Zone: %d   ", (int)irrigationMon_.irrigation.FrontActiveZone);
         }
         else if (irrigationMon_.irrigation.BackControllerActive == 1) {
            fprintf(fptr, "Controller: Back  ");
            fprintf(fptr, "Zone: %d   ", (int)irrigationMon_.irrigation.BackActiveZone);
         }
         else {
            fprintf(fptr, "Controller: ??  ");
            fprintf(fptr, "Zone: ??   ");
         }
         fprintf(fptr, "Gallons Used: %d   ", stopGallons);
         fprintf(fptr, "Run Time: %f  Min. ", (diff_t/60));
         fprintf(fptr, "%s", ctime(&t));
         fclose(fptr);
         lastpumpState = OFF ;
         sleep(1);
         irrigationMon_.irrigation.FrontControllerActive  =    0;  
         irrigationMon_.irrigation.FrontActiveZone        =    0;  
         irrigationMon_.irrigation.BackControllerActive   =    0;  
         irrigationMon_.irrigation.BackActiveZone         =    0;  
      }
      if ((rainbirdRequest == TRUE) && (rainbirdReqDelay == 0)){
         pubmsg.payload = rainbird_command1;
         pubmsg.payloadlen = strlen(rainbird_command1);
         pubmsg.qos = 0;
         pubmsg.retained = 0;
         deliveredtoken = 0;
         if ((rc = MQTTClient_publishMessage(client, RAINBIRDCOMMAND_TOPICID, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
         {
            printf("Failed to publish rainbird command message for controller 1, return code %d\n", rc);
            log_message("IrrigationMonitor Error == Failed to Publish Rainbird Command Message for controller 1. Return Code: %d\n", rc);
            rc = EXIT_FAILURE;
         }
         pubmsg.payload = rainbird_command2;
         pubmsg.payloadlen = strlen(rainbird_command2);
         pubmsg.qos = 0;
         pubmsg.retained = 0;
         deliveredtoken = 0;
         if ((rc = MQTTClient_publishMessage(client, RAINBIRDCOMMAND_TOPICID, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
         {
            printf("Failed to publish rainbird command message for controller 2, return code %d\n", rc);
            log_message("IrrigationMonitor Error == Failed to Publish Rainbird Command Message for controller 2. Return Code: %d\n", rc);
            rc = EXIT_FAILURE;
         }
         rainbirdRecvRequest = TRUE; 
         rainbirdReqDelay = RAINBIRD_REQUEST_DELAY;
         rainbirdRequest = FALSE;
      } else if (rainbirdRequest == TRUE) {
         rainbirdReqDelay--; 
      }
      if (training_mode && pumpState == ON) {
         FILE *file = fopen(training_filename, "a");
         if (file != NULL) {
            time_t current_time = time(NULL);
            fprintf(file, "%ld, %f, %f, %f\n", current_time, wellMon_.well.amp_pump_4, avgflowRateGPM, irrigationPressure);
            fclose(file);
         } else {
            fprintf(stderr, "Error opening file: %s\n", training_filename);
         }
      }
      sleep(1) ;
   }
   
   log_message("IrrigationMonitor Exited Main Loop\n");
   MQTTClient_unsubscribe(client, IRRIGATIONMON_TOPICID);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
