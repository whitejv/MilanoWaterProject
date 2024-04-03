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

/* Function Declarations */


void GallonsPumped(void) ;
void PumpStats(void) ;
void MyMQTTSetup(char *mqtt_address) ;
void MyMQTTPublish(void) ;
float moving_average(float new_sample, float samples[], uint8_t *sample_index, uint8_t window_size) ;

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
 

   float temperatureF;
  
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

   log_message("HouseMonitor: Started\n");

   /*
   * MQTT Setup
   */

   MyMQTTSetup(mqtt_address);
   
   /*
    * Main Loop
    */

   log_message("HouseMonitor: Entering Main Loop\n") ;
   
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
         
         fprintf(fptr, "Daily House Gallons Used: %f %s", dailyGallons, ctime(&t));
         dailyGallons = 0; 
         fclose(fptr);
         
      }
      //printf("seconds since midnight: %d\n", SecondsFromMidnight);
      PriorSecondsFromMidnight = SecondsFromMidnight ;

      GallonsPumped() ;
      
      memcpy(&temperatureF, &houseSens_.house.temp_w1 , sizeof(float));

      houseMon_.house.houseSupplyTemp =  temperatureF;
      houseMon_.house.cycle_count =    houseSens_.house.cycle_count;
      houseMon_.house.housePressure = houseSens_.house.adc_sensor * 0.0048828125 * 100;

      MyMQTTPublish() ;

      PumpStats() ;
   /*
    * Run at this interval
    */
   
      sleep(1) ;
   }
   
   log_message("HouseMonitor: Exited Main Loop\n");
   MQTTClient_unsubscribe(client, HOUSEMON_TOPICID);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}

void GallonsPumped(void){
   int i=0;
   float calibrationFactor = .5;
   float flowRate = 0.0;
   
   float flowRateGPM = 0;
   static float avgflowRateGPM = 0;
   static float avgflowRate = 0;
   static int   flowIndex = 0;
   static float flowRateValueArray[10] = {0.,0.,0.,0.,0.,0.,0.,0.,0.,0.};
   int pulseCount = 0;
   int millsElapsed = 0;
   int millsTotal = 0;
   int dailyPulseCount = 0;
   int newPulseData = 0;

   newPulseData = houseSens_.house.new_data_flag;
   if ( newPulseData == 1){
      
      millsElapsed = houseSens_.house.milliseconds;
      pulseCount = houseSens_.house.pulse_count;
      
      if ((millsElapsed < 5000) && (millsElapsed != 0)) {     //ignore the really long intervals
         //dailyPulseCount = dailyPulseCount + pulseCount ;
         millsElapsed = houseSens_.house.milliseconds ;
         //millsTotal = millsTotal + millsElapsed;
         flowRate = ((pulseCount / (millsElapsed/1000)) / .5) / calibrationFactor;
         flowRate = ((flowRate * .00026417)/(millsElapsed/1000)) * 60;  //GPM
         flowRateGPM = flowRate * 30;
         dailyGallons = dailyGallons + flowRate ;
         
         if (flowRateGPM > 4.0) {
            flowRateValueArray[flowIndex++] = flowRateGPM;
            flowIndex = flowIndex % 10;
         }
         avgflowRate = 0 ;
         for( i=0; i<=9; ++i){
            avgflowRate += flowRateValueArray[i];
            //printf("flowRateValueArray[%d]: %f avgflowRate: %f\n", i, flowRateValueArray[i], avgflowRate );
         }
         avgflowRateGPM = avgflowRate/10;
         
         /*
         printf("Pulse Count: %d   Daily Pulse Count: %d\n", pulseCount, dailyPulseCount);
         printf("Milliseconds Elapsed: %d   Milliseconds Total:  %d\n", millsElapsed, millsTotal);
         printf("Flow Rate: %f  Flow Rate GPM:  %f   Daily Gallons:  %f\n", flowRate, flowRateGPM,  dailyGallons);
         printf("Average Flow Rate: %f\n", avgflowRateGPM);
         */  
      }    
   } 
   else {
      pulseCount = 0;
      millsElapsed = 0 ;
   }

   houseMon_.house.house_gallons_per_minute =    avgflowRateGPM;
   houseMon_.house.houseTotalFlow = dailyGallons;
}

void MyMQTTSetup(char* mqtt_address){

   int rc;

   if ((rc = MQTTClient_create(&client, mqtt_address, HOUSEMON_CLIENTID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to create client, return code %d\n", rc);
      log_message("HouseMonitor: Error == Failed to Create Client. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to set callbacks, return code %d\n", rc);
      log_message("HouseMonitor: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
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
      log_message("HouseMonitor: Error == Failed to Connect. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", HOUSESENS_TOPICID, HOUSESENS_CLIENTID, QOS);
   log_message("HouseMonitor: Subscribing to topic: %s for client: %s\n", HOUSESENS_TOPICID, HOUSESENS_CLIENTID);
   MQTTClient_subscribe(client, HOUSESENS_TOPICID, QOS);

   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", WELLMON_TOPICID, WELLMON_CLIENTID,  QOS);
   log_message("HouseMonitor: Subscribing to topic: %s for client: %s\n", WELLMON_TOPICID, WELLMON_CLIENTID );
   MQTTClient_subscribe(client, WELLMON_TOPICID, QOS);
}
void MyMQTTPublish() {
   int rc;
   int i;
   time_t t;
   time(&t);

   if (verbose) {
      for (i=0; i<=HOUSEMON_LEN-1; i++) {
         printf("%f ", houseMon_.data_payload[i]);
      }
      printf("%s", ctime(&t));
   }
   pubmsg.payload = houseMon_.data_payload;
   pubmsg.payloadlen = HOUSEMON_LEN * 4;
   pubmsg.qos = QOS;
   pubmsg.retained = 0;
   deliveredtoken = 0;
   if ((rc = MQTTClient_publishMessage(client, HOUSEMON_TOPICID, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to publish message, return code %d\n", rc);
      log_message("HouseMonitor: Error == Failed to Publish Message. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
   }


json_object *root = json_object_new_object();
for (i=0; i<=HOUSEMON_LEN-1; i++) {
   json_object_object_add(root, housemon_ClientData_var_name [i], json_object_new_double(houseMon_.data_payload[i]));
}

const char *json_string = json_object_to_json_string(root);

pubmsg.payload = (void *)json_string; // Make sure to cast the const pointer to void pointer
pubmsg.payloadlen = strlen(json_string);
pubmsg.qos = QOS;
pubmsg.retained = 0;
MQTTClient_publishMessage(client, HOUSEMON_JSONID, &pubmsg, &token);
//printf("Waiting for publication of %s\non topic %s for client with ClientID: %s\n", json_string, HOUSE_TOPIC, HOUSE_MONID);
MQTTClient_waitForCompletion(client, token, TIMEOUT);
//printf("Message with delivery token %d delivered\n", token);

json_object_put(root); // Free the memory allocated to the JSON object

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
int housestartGallons = 0;
int housestopGallons = 0;

if (wellMon_.well.well_pump_1_on  == 1  || wellMon_.well.well_pump_2_on == 1) {
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
