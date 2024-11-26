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

float dailyGallons = 0;
float TotalDailyGallons = 0;
float TotalGPM = 0;
float avgflowRateGPM = 0;

#define SAMPLES_COUNT 10
float samples[SAMPLES_COUNT] = {0};
uint8_t sample_index = 0;
uint8_t window_size = 10; // Change this value to the desired window size (60-100)
float PresSensorValue = 0;

/* Function Declarations */

void GallonsPumped(void) ;
void PumpStats(void) ;
void MyMQTTSetup(char *mqtt_address) ;
void MyMQTTPublish(void) ;
float moving_average(float new_sample, float samples[], uint8_t *sample_index, uint8_t window_size) ;
void publishLogMessage(union LOG_ *log_data, const char *message_id);

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;MQTTClient_deliveryToken deliveredtoken;

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
   int rc;
   FILE *fptr;
   time_t t;
   time_t start_t, end_t;
   double diff_t;
   struct tm timenow;
   time(&t);

   int SecondsFromMidnight = 0 ;
   int PriorSecondsFromMidnight =0;
// Add a static variable to track the previous timestamp
   static struct timespec previous_time = {0, 0}; // Initialized to 0 at the start
   struct timespec current_time;
   
   float temperatureF;
   float intervalFlow = 0; 
   float TankGallons = 0;


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

   log_message("well3Monitor: Started\n");

   /*
   * MQTT Setup
   */

   MyMQTTSetup(mqtt_address);
   
   /*
    * Main Loop
    */

   log_message("well3Monitor: Entering Main Loop\n") ;
   
   while(1)
   {
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
         fprintf(fptr, "Daily well3 Gallons Used: %f %s", dailyGallons, ctime(&t));
         dailyGallons = 0; 
         fclose(fptr);
      }

      flowmon(well3Sens_.well3.new_data_flag, well3Sens_.well3.milliseconds, well3Sens_.well3.pulse_count, &avgflowRateGPM, &intervalFlow, .98) ;
      dailyGallons = dailyGallons + intervalFlow;
      well3Mon_.well3.intervalFlow = intervalFlow;
      well3Mon_.well3.amperage = wellMon_.well.amp_pump_3;
      well3Mon_.well3.gallonsMinute =  avgflowRateGPM;
      well3Mon_.well3.gallonsDay = dailyGallons;
      well3Mon_.well3.controller = 3;
      well3Mon_.well3.zone = 1;
      
      memcpy(&temperatureF, &well3Sens_.well3.temp_w1 , sizeof(float));

      well3Mon_.well3.temperatureF =  temperatureF;
      well3Mon_.well3.cycleCount =    well3Sens_.well3.cycle_count;
      //well3Mon_.well3.well3Pressure = well3Sens_.well3.adc_sensor * 0.0048828125 * 20;
      //using moving average for now

      PresSensorValue = ((well3Sens_.well3.adc_sensor * 0.107)-12.164);
            
      well3Mon_.well3.pressurePSI = moving_average(PresSensorValue, samples, &sample_index, window_size);
      
      MyMQTTPublish() ;

      PumpStats() ;

#include <time.h>

// Inside your loop or main function
if (wellMon_.well.well_pump_3_on == 1) {
    
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    if (previous_time.tv_sec != 0 || previous_time.tv_nsec != 0) {
        // Calculate the time slice in seconds since the last execution
        well3Mon_.well3.secondsOn = (float)(current_time.tv_sec - previous_time.tv_sec) +
                                    (float)(current_time.tv_nsec - previous_time.tv_nsec) / 1.0e9f;
    } else {
        // This is the first execution; initialize secondsOn to 0
        well3Mon_.well3.secondsOn = 0.0f;
    }

    // Update the previous timestamp to the current time
    previous_time = current_time;

    // Populate the log structure
    log_.log.Controller = well3Mon_.well3.controller;
    log_.log.Zone = well3Mon_.well3.zone;
    log_.log.pressurePSI = well3Mon_.well3.pressurePSI;
    log_.log.temperatureF = well3Mon_.well3.temperatureF;
    log_.log.intervalFlow = well3Mon_.well3.intervalFlow;
    log_.log.amperage = well3Mon_.well3.amperage;
    log_.log.secondsOn = well3Mon_.well3.secondsOn;

    publishLogMessage(&log_, "well3");
} else {
    // If the pump is not running, set secondsOn to 0
    well3Mon_.well3.secondsOn = 0.0f;

    // Optionally update previous_time to avoid long intervals when the pump starts again
    clock_gettime(CLOCK_MONOTONIC, &previous_time);
}

   /*
    * Run at this interval
    */
   
      sleep(1) ;
   }
   
   log_message("well3Monitor: Exited Main Loop\n");
   MQTTClient_unsubscribe(client, WELL3MON_TOPICID);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}

void MyMQTTSetup(char* mqtt_address){

   int rc;

   if ((rc = MQTTClient_create(&client, mqtt_address, WELL3MON_CLIENTID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to create client, return code %d\n", rc);
      log_message("well3Monitor: Error == Failed to Create Client. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to set callbacks, return code %d\n", rc);
      log_message("well3Monitor: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
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
      log_message("well3Monitor: Error == Failed to Connect. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", WELL3SENS_TOPICID, WELL3SENS_CLIENTID, QOS);
   log_message("well3Monitor: Subscribing to topic: %s for client: %s\n", WELL3SENS_TOPICID, WELL3SENS_CLIENTID);
   MQTTClient_subscribe(client, WELL3SENS_TOPICID, QOS);

   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", WELLMON_TOPICID, WELLMON_CLIENTID,  QOS);
   log_message("well3Monitor: Subscribing to topic: %s for client: %s\n", WELLMON_TOPICID, WELLMON_CLIENTID );
   MQTTClient_subscribe(client, WELLMON_TOPICID, QOS);
}
void MyMQTTPublish() {
   int rc;
   int i;
   time_t t;
   time(&t);

   if (verbose) {
      for (i=0; i<=WELL3MON_LEN-1; i++) {
         printf("%f ", well3Mon_.data_payload[i]);
      }
      printf("%s", ctime(&t));
   }
   pubmsg.payload = well3Mon_.data_payload;
   pubmsg.payloadlen = WELL3MON_LEN * 4;
   pubmsg.qos = QOS;
   pubmsg.retained = 0;
   deliveredtoken = 0;
   if ((rc = MQTTClient_publishMessage(client, WELL3MON_TOPICID, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to publish message, return code %d\n", rc);
      log_message("well3Monitor: Error == Failed to Publish Message. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
   }


json_object *root = json_object_new_object();
for (i=0; i<=WELL3MON_LEN-1; i++) {
   json_object_object_add(root, well3mon_ClientData_var_name [i], json_object_new_double(well3Mon_.data_payload[i]));
}

const char *json_string = json_object_to_json_string(root);

pubmsg.payload = (void *)json_string; // Make sure to cast the const pointer to void pointer
pubmsg.payloadlen = strlen(json_string);
pubmsg.qos = QOS;
pubmsg.retained = 0;
MQTTClient_publishMessage(client, WELL3MON_JSONID, &pubmsg, &token);
//printf("Waiting for publication of %s\non topic %s for client with ClientID: %s\n", json_string, well3_TOPIC, well3_MONID);
MQTTClient_waitForCompletion(client, token, TIMEOUT);
//printf("Message with delivery token %d delivered\n", token);

json_object_put(root); // Free the memory allocated to the JSON object

}
void publishLogMessage(union LOG_ *log_data, const char *message_id) {
   json_object *root = json_object_new_object();
   char topic[100];
   // Create JSON object with proper types
   // First two fields are integers
   json_object_object_add(root, log_ClientData_var_name[0], 
      json_object_new_int(log_data->log.Controller));
   json_object_object_add(root, log_ClientData_var_name[1], 
      json_object_new_int(log_data->log.Zone));  
   json_object_object_add(root, log_ClientData_var_name[2], 
      json_object_new_double(log_data->log.pressurePSI));
   json_object_object_add(root, log_ClientData_var_name[3], 
      json_object_new_double(log_data->log.temperatureF));
   json_object_object_add(root, log_ClientData_var_name[4], 
      json_object_new_double(log_data->log.intervalFlow));
   json_object_object_add(root, log_ClientData_var_name[5], 
      json_object_new_double(log_data->log.amperage));
   json_object_object_add(root, log_ClientData_var_name[6], 
      json_object_new_double(log_data->log.secondsOn));

   const char *json_string = json_object_to_json_string(root);
   
   // Create topic string with message_id
   snprintf(topic, sizeof(topic), "%s%s/", LOG_JSONID, message_id);
   
   // Prepare and publish MQTT message
   pubmsg.payload = (void *)json_string;
   pubmsg.payloadlen = strlen(json_string);
   pubmsg.qos = QOS;
   pubmsg.retained = 0;
   
   MQTTClient_publishMessage(client, topic, &pubmsg, &token);
   MQTTClient_waitForCompletion(client, token, TIMEOUT);
   
   json_object_put(root);
} 
void PumpStats() {

FILE *fptr;
time_t t;
time_t start_t, end_t;
double diff_t;

time(&t);
int pumpState = 0;
int lastpumpState = 0;
int startGallons = 0;
int stopGallons = 0;
int well3startGallons = 0;
int well3stopGallons = 0;

if (wellMon_.well.well_pump_3_on  == 1) {
      pumpState = ON;
   }
   else {
      pumpState = OFF;
   }
   
   if ((pumpState == ON) && (lastpumpState == OFF)){
      startGallons = dailyGallons;
      //tankstartGallons = tankmon_data_payload[2];
      time(&start_t);
      lastpumpState = ON;
   }
   else if ((pumpState == OFF) && (lastpumpState == ON)){
      fptr = fopen(flowdata, "a");
      stopGallons = dailyGallons - startGallons ;
      //tankstopGallons = tankmon_data_payload[2];
      time(&end_t);
      diff_t = difftime(end_t, start_t);
      fprintf(fptr, "Last Pump Cycle Gallons Used: %d   ", stopGallons);
      fprintf(fptr, "Run Time: %f  Min. ", (diff_t/60));
      fprintf(fptr, "Well Gallons Pumped: %d  ", (stopGallons-startGallons));
      fprintf(fptr, "%s", ctime(&t));
      fclose(fptr);
      //TotalDailyGallons = TotalDailyGallons + (stopGallons-startGallons);
      lastpumpState = OFF ;
   }
}
float moving_average(float new_sample, float samples[], uint8_t *sample_index, uint8_t window_size) {
    static float sum = 0;
    static uint8_t n = 0;

    // Remove the oldest sample from the sum
    sum -= samples[*sample_index];

    // Add the new sample to the sum
    sum += new_sample;

    // Replace the oldest sample with the new sample
    samples[*sample_index] = new_sample;

    // Update the sample index
    *sample_index = (*sample_index + 1) % window_size;

    // Update the number of samples, up to the window size
    if (n < window_size) {
        n++;
    }

    // Calculate and return the moving average
    return sum / n;
}