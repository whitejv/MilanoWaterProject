#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"


float TotalDailyGallons = 0;
float TotalGPM = 0;

/* Kalman Filter Setup */
#define DT 0.1  // Time step
#define A 1     // Matrix A
#define H 1     // Matrix H
#define Q 0.001 // Process noise covariance
#define R 0.05   // Measurement noise covariance

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
   
   log_message("TankMonitor: Started\n");

   if ((rc = MQTTClient_create(&client, ADDRESS, TANK_MONID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to create client, return code %d\n", rc);
      log_message("TankMonitor: Error == Failed to Create Client. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to set callbacks, return code %d\n", rc);
      log_message("TankMonitor: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
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
      log_message("TankMonitor: Error == Failed to Connect. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", TANK_CLIENT, TANK_CLIENTID, QOS);
   log_message("TankMonitor: Subscribing to topic: %s for client: %s\n", TANK_CLIENT, TANK_CLIENTID);
   MQTTClient_subscribe(client, TANK_CLIENT, QOS);
   
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", WELL_TOPIC, WELL_MONID, QOS);
   log_message("TankMonitor: Subscribing to topic: %s for client: %s\n", WELL_TOPIC, WELL_MONID);
   MQTTClient_subscribe(client, WELL_TOPIC, QOS);

   
   /*
    * Main Loop
    */

   log_message("TankMonitor: Entering Main Loop\n") ;
   
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
      
      void GallonsInTank(void)
 
      Float100State = tank_data_payload[6] ;
      Float90State = tank_data_payload[7]  ;
      Float50State = tank_data_payload[4]  ;
      Float25State = tank_data_payload[5]  ;

      
      
      newPulseData = tank_data_payload[2] ;
      if ( newPulseData == 1){
         
         millsElapsed = tank_data_payload[1] ;
         pulseCount = tank_data_payload[0];
         
         if ((millsElapsed < 5000) && (millsElapsed != 0)) {     //ignore the really long intervals
            //dailyPulseCount = dailyPulseCount + pulseCount ;
            millsElapsed = tank_data_payload[1] ;
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
      
      //temperatureF = *((float *)&flow_data_payload[17]);
      
      memcpy(&temperatureF, &tank_data_payload[17], sizeof(float));
      
      
      /*
       * Log the Data Based on Pump 4 on/off
       */
      
      /*
       * Load Up the Data
       */
      
      /* CLIENTID     "Tank Subscriber", TOPIC "flow Data", flow_sensor_ */
      tank_sensor_payload[0] =    avgflowRateGPM;
      tank_sensor_payload[1] =    WaterHeight;
      tank_sensor_payload[2] =    TankGallons;
      tank_sensor_payload[3] =    TankPerFull;
      tank_sensor_payload[4] =    dailyGallons;
      tank_sensor_payload[5] =    0;
      tank_sensor_payload[6] =    0;
      tank_sensor_payload[7] =    0;
      tank_sensor_payload[8] =    0;
      tank_sensor_payload[9] =    0;
      tank_sensor_payload[10] =   tank_data_payload[12];
      tank_sensor_payload[11] =   temperatureF;
      tank_sensor_payload[12] =   Float100State;
      tank_sensor_payload[13] =   Float90State;
      tank_sensor_payload[14] =   Float50State;
      tank_sensor_payload[15] =   Float25State;
      tank_sensor_payload[16] =   0;
      tank_sensor_payload[17] =   0;
      tank_sensor_payload[18] =   0;
      tank_sensor_payload[19] =   0;
      /*
      for (i=0; i<=TANK_DATA; i++) {
          printf("%f ", tank_sensor_payload[i]);
      }
      printf("%s", ctime(&t));
      */
      pubmsg.payload = tank_sensor_payload;
      pubmsg.payloadlen = TANK_DATA * 4;
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      deliveredtoken = 0;
      if ((rc = MQTTClient_publishMessage(client, TANK_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
      {
         printf("Failed to publish message, return code %d\n", rc);
         log_message("TankMonitor: Error == Failed to Publish Message. Return Code: %d\n", rc);
         rc = EXIT_FAILURE;
      }
      
      /*
       * Run at this interval
       */
      
      if (well_sensor_payload[2] == 1) {
         pumpState = ON;
      }
      else {
         pumpState = OFF;
      }
      
      if ((pumpState == ON) && (lastpumpState == OFF)){
         startGallons = dailyGallons;
         //tankstartGallons = tank_sensor_payload[2];
         time(&start_t);
         lastpumpState = ON;
      }
      else if ((pumpState == OFF) && (lastpumpState == ON)){
         fptr = fopen(flowdata, "a");
         stopGallons = dailyGallons - startGallons ;
         //tankstopGallons = tank_sensor_payload[2];
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
      
      sleep(1) ;
   }
   
   log_message("TankMonitor: Exited Main Loop\n");
   MQTTClient_unsubscribe(client, TANK_TOPIC);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
void GallonsInTank() {
   float PresSensorLSB = .00322580645; // lsb voltage value from datasheet
  
   float PresSensorValue = 0;
   float PresSensorRawValue = 0;

   float ConstantX = .34; // Used Excel Polynomial Fitting to come up with equation
   float Constant = .0962;
  
   float WaterHeight = 0;
   float TankGallons = 0;
   float TankPerFull = 0;
   float Tank_Area = 0;

   // Set initial state and state covariance for Kalman filter
   x = 0;
   P = 1;
   /*
   * Convert Raw hydrostatic Pressure Sensor
   * A/D to Water Height, Gallons & Percent Full
   */

   PresSensorRawValue = tank_data_payload[3] * PresSensorLSB;
   
   /*
      * Kalman Filter to smooth the hydrostatic sensor readings
      */

   z = PresSensorRawValue;
   printf("z: %f\n", z);  // Print updated state
   predict();
   update();
   printf("x: %f\n", x);  // Print updated state
   PresSensorValue = x;
   
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
}
