#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"


/* payload 0    Pulses Counted in Time Window
 * payload 1    Number of milliseconds in Time Window
 * payload 2    Flag 1=new data 0=stale data
 * payload 3    Pressure Sensor Analog Value
 * payload 4     unused
 * payload 5     unused
 * payload 6     unused
 * payload 7     unused
 * payload 8     unused
 * payload 9     unused
 * payload 10     unused
 * payload 11     unused
 * payload 12     Cycle Counter 16bit Int
 * payload 13     unused
 * payload 14     unused
 * payload 15     unused
 * payload 16     unused
 * payload 17    Temperature in F
 * payload 18     unused
 * payload 19     unused
 * payload 20     FW Version 4 Hex
 */
/*
 * payload 21     Last payload is Control Word From User
 */

/* payload[0] =    Gallons Per Minute
 * payload[1] =    Total Gallons (24 Hrs)
 * payload[2] =    Irrigation Pressure
 * payload[3] =    Pump Temperature
 * payload[4] =    spare
 * payload[5] =    spare
 * payload[6] =    spare
 * payload[7] =    spare
 * payload[8] =    spare
 * payload[9] =    spare
 * payload[10] =    Cycle Count
 * payload[11] =    spare
 * payload[12] =    spare
 * payload[13] =    spare
 * payload[14] =    spare
 * payload[15] =    spare
 * payload[16] =    spare
 * payload[17] =    spare
 * payload[18] =    spare
 * payload[19] =    spare
 * payload[20] =    spare
 */

#define flowfile "./flowfile.txt"
#define flowdata "./flowdata.txt"

float TotalDailyGallons = 0;
float TotalGPM = 0;


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
   float calibrationFactor = .5;
   float flowRate = 0.0;
   float dailyGallons = 0;
   float flowRateGPM = 0;
   float avgflowRateGPM = 0;
   float avgflowRate = 0;
   static int   flowIndex = 0;
   static float flowRateValueArray[10] = {0.,0.,0.,0.,0.,0.,0.,0.,0.,0.};
   int pulseCount = 0;
   int millsElapsed = 0;
   int millsTotal = 0;
   int dailyPulseCount = 0;
   int newPulseData = 0;
   int pumpState = 0;
   int lastpumpState = 0;
   int startGallons = 0;
   int stopGallons = 0;
   int tankstartGallons = 0;
   int tankstopGallons = 0;
   
   
   MQTTClient client;
   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   int rc;
   
   log_message("FlowMonitor: Started\n");

   if ((rc = MQTTClient_create(&client, ADDRESS, FLO_CLIENTID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to create client, return code %d\n", rc);
      log_message("FlowMonitor: Error == Failed to Create Client. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to set callbacks, return code %d\n", rc);
      log_message("FlowMonitor: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
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
      log_message("FlowMonitor: Error == Failed to Connect. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", FLO_TOPIC, FLO_CLIENTID, QOS);
   log_message("FlowMonitor: Subscribing to topic: %s for client: %s\n", FLO_TOPIC, FLO_CLIENTID);
   MQTTClient_subscribe(client, FLO_TOPIC, QOS);
   
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", F_TOPIC, F_CLIENTID, QOS);
   log_message("FlowMonitor: Subscribing to topic: %s for client: %s\n", F_TOPIC, F_CLIENTID);
   MQTTClient_subscribe(client, F_TOPIC, QOS);
   
   /*
    * Main Loop
    */

   log_message("FlowMonitor: Entering Main Loop\n") ;
   
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
         
         fprintf(fptr, "Daily Gallons Used: %f %s", dailyGallons, ctime(&t));
         dailyGallons = 0; 
         fclose(fptr);
         
      }
      //printf("seconds since midnight: %d\n", SecondsFromMidnight);
      PriorSecondsFromMidnight = SecondsFromMidnight ;
      
      
      
      /*
       * Compute Monitor Values Based on Inputs from
       * Sensor Data and Format for easy use with Blynk
       */
      
      
      newPulseData = flow_data_payload[2] ;
      if ( newPulseData == 1){
         
         millsElapsed = flow_data_payload[1] ;
         pulseCount = flow_data_payload[0];
         
         if ((millsElapsed < 5000) && (millsElapsed != 0)) {     //ignore the really long intervals
            //dailyPulseCount = dailyPulseCount + pulseCount ;
            millsElapsed = flow_data_payload[1] ;
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
      } else {
         pulseCount = 0;
         millsElapsed = 0 ;
      }
      
      
      irrigationPressure = (flow_data_payload[3] * .9756525)/10 ;
      
      //temperatureF = *((float *)&flow_data_payload[17]);
      
      memcpy(&temperatureF, &flow_data_payload[17], sizeof(float));
      
      
      /*
       * Log the Data Based on Pump 4 on/off
       */
      
      /*
       * Load Up the Data
       */
      
      /* CLIENTID     "Tank Subscriber", TOPIC "flow Data", flow_sensor_ */
      flow_sensor_payload[0] =    avgflowRateGPM;
      flow_sensor_payload[1] =    dailyGallons;
      flow_sensor_payload[2] =    irrigationPressure;
      flow_sensor_payload[3] =    temperatureF;
      flow_sensor_payload[4] =    0;
      flow_sensor_payload[5] =    0;
      flow_sensor_payload[6] =    0;
      flow_sensor_payload[7] =    0;
      flow_sensor_payload[8] =    0;
      flow_sensor_payload[9] =    0;
      flow_sensor_payload[10] =   flow_data_payload[12];
      flow_sensor_payload[11] =   0;
      flow_sensor_payload[12] =   0;
      flow_sensor_payload[13] =   0;
      flow_sensor_payload[14] =   0;
      flow_sensor_payload[15] =   0;
      flow_sensor_payload[16] =   0;
      flow_sensor_payload[17] =   0;
      flow_sensor_payload[18] =   0;
      flow_sensor_payload[19] =   0;
      /*
      for (i=0; i<=FL_LEN; i++) {
          printf("%f ", flow_sensor_payload[i]);
      }
      printf("%s", ctime(&t));
      */
      pubmsg.payload = flow_sensor_payload;
      pubmsg.payloadlen = FL_LEN * 4;
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      deliveredtoken = 0;
      if ((rc = MQTTClient_publishMessage(client, FL_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
      {
         printf("Failed to publish message, return code %d\n", rc);
         log_message("FlowMonitor: Error == Failed to Publish Message. Return Code: %d\n", rc);
         rc = EXIT_FAILURE;
      }
      
      /*
       * Run at this interval
       */
      
      if (formatted_sensor_payload[7] > 500) {
         pumpState = ON;
      }
      else {
         pumpState = OFF;
      }
      
      if ((pumpState == ON) && (lastpumpState == OFF)){
         startGallons = dailyGallons;
         tankstartGallons = formatted_sensor_payload[2];
         time(&start_t);
         lastpumpState = ON;
      }
      else if ((pumpState == OFF) && (lastpumpState == ON)){
         fptr = fopen(flowdata, "a");
         stopGallons = dailyGallons - startGallons ;
         tankstopGallons = formatted_sensor_payload[2];
         time(&end_t);
         diff_t = difftime(end_t, start_t);
         fprintf(fptr, "Last Pump Cycle Gallons Used: %d   ", stopGallons);
         fprintf(fptr, "Run Time: %f  Min. ", (diff_t/60));
         fprintf(fptr, "Tank Gallons Used: %d  ", (tankstartGallons-tankstopGallons));
         fprintf(fptr, "%s", ctime(&t));
         fclose(fptr);
         lastpumpState = OFF ;
      }
      
      sleep(1) ;
   }
   
   log_message("FlowMonitor: Exited Main Loop\n");
   MQTTClient_unsubscribe(client, F_TOPIC);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
