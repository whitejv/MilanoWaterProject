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
float dailyGallons = 0;
float TotalDailyGallons = 0;
float TotalGPM = 0;
float avgflowRateGPM = 0;

int pumpState = 0;
int lastpumpState = 0;
int startGallons = 0;
int stopGallons = 0;
int tankstartGallons = 0;
int tankstopGallons = 0;

typedef struct {
    double value;  // Fused measurement value
    double error;  // Measurement error
} FusedMeasurement;

typedef struct {
    double gain;   // Kalman gain
    double value;  // Estimated measurement value
    double error;  // Estimation error
} KalmanFilter;

FusedMeasurement fused;
/* Function Declarations */

float GallonsInTankPress(void) ;
float GallonsInTankUltra(void) ;
FusedMeasurement fuseMeasurements(double measurement1, double measurement2, double measurementError1, double measurementError2);
void GallonsPumped(void) ;
void PumpStats(void) ;
void MyMQTTSetup(char *mqtt_address) ;
void MyMQTTPublish(void) ;
float moving_average(float new_sample, float samples[], uint8_t *sample_index, uint8_t window_size) ;

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
   float intervalFlow = 0;
   float waterHeightPress = 0;
   float waterHeightUltra = 0;
   float temperatureF;
   float waterHeight = 0;
   float TankGallons = 0;
   float TankPerFull = 0;
   float Tank_Area = 0;
   int Float100State = 0;
   int Float25State = 0;

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
               fprintf(stderr, "Usage: %s [-v] [-P | -D] [-T filename\n", argv[0]);
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

   log_message("TankMonitor: Started\n");

   /*
   * MQTT Setup
   */

   MyMQTTSetup(mqtt_address);
   
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
      
      waterHeightPress = GallonsInTankPress() ;
   
      //printf("Water Height Press: %f\n", waterHeightPress);

      waterHeight = waterHeightPress;
      /*
       * Use the Equation (PI*R^2*WaterHeight)*VoltoGal to compute Water Gallons in tank
      */
      Tank_Area = PI * Tank_Radius_sqd; // area of base of tank
      TankGallons = ((Tank_Area)*waterHeight) * VoltoGal;
      // printf("Gallons in Tank = %f\n", TankGallons);
      TankPerFull = TankGallons / MaxTankGal * 100;
      // printf("Percent Gallons in Tank = %f\n", TankPerFull);
     
     
      tankMon_.tank.water_height =  waterHeight;
      tankMon_.tank.tank_gallons =  TankGallons;
      tankMon_.tank.tank_per_full =  TankPerFull;

      flowmon(tankSens_.tank.new_data_flag, tankSens_.tank.milliseconds, tankSens_.tank.pulse_count, &avgflowRateGPM, &intervalFlow, .98) ;
      dailyGallons = dailyGallons + intervalFlow;
      tankMon_.tank.intervalFlow = intervalFlow;
      tankMon_.tank.gallonsMinute =  avgflowRateGPM;
      tankMon_.tank.gallonsDay =    dailyGallons;
      tankMon_.tank.controller = 3;
      tankMon_.tank.zone = 1;
      
      memcpy(&temperatureF, &tankSens_.data_payload[6], sizeof(float));
      tankMon_.tank.temperatureF =    temperatureF;

      Float100State = tankSens_.tank.gpio_sensor & 0x01;
      Float25State = (tankSens_.tank.gpio_sensor & 0x02) >> 1;
      
      tankMon_.tank.float1 =    Float100State;
      tankMon_.tank.float2 =    Float25State;
      tankMon_.tank.cycleCount =  tankSens_.tank.cycle_count ;
      tankMon_.tank.fwVersion = 0;
      
      MyMQTTPublish() ;

      PumpStats() ;
      if (training_mode && pumpState == ON) {
         FILE *file = fopen(training_filename, "a");
         if (file != NULL) {
            time_t current_time = time(NULL);
            fprintf(file, "%ld, %f, %f, %f\n", current_time, wellMon_.well.amp_pump_3, avgflowRateGPM, 1.0);
            fclose(file);
         } else {
            fprintf(stderr, "Error opening file: %s\n", training_filename);
         }
      }
   /*
    * Run at this interval
    */
   
      sleep(1) ;
   }
   
   log_message("TankMonitor: Exited Main Loop\n");
   MQTTClient_unsubscribe(client, TANKMON_TOPICID);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}

#define SAMPLES_COUNT 50
float samples[SAMPLES_COUNT] = {0};
uint8_t sample_index = 0;
uint8_t window_size = 40; // Change this value to the desired window size (60-100)

float GallonsInTankPress(void) {
   float waterHeight = 0;
   float PresSensorLSB = .00322580645; // lsb voltage value from datasheet
  
   float PresSensorValue = 0;
   float PresSensorRawValue = 0;

   float ConstantX = 4.7359; // Used Excel Polynomial Fitting to come up with equation
   float Constant = 3.6869;

   // Set initial state and state covariance for Kalman filter
   x = 0;
   P = 1;
   /*
   * Convert Raw hydrostatic Pressure Sensor
   * A/D to Water Height, Gallons & Percent Full
   */

   PresSensorRawValue = tankSens_.tank.adc_sensor * PresSensorLSB;
   
   /*
      * Kalman Filter to smooth the hydrostatic sensor readings
      */

   z = PresSensorRawValue;
   //printf("z: %f\n", z);  // Print updated state
   predict();
   update();
   //printf("x: %f\n", x);  // Print updated state
   PresSensorValue = x;
           
   PresSensorValue = moving_average(x, samples, &sample_index, window_size);
   //printf("Sample: %f, Moving average: %f\n", x, PresSensorValue);
   /*
      *** Use the Equation y=Constandx(x) + Constant solve for x to compute Water Height in tank
      */
/*
   if (PresSensorValue <= Constant)
   {
      PresSensorValue = Constant;
   }
*/
   //WaterHeight = ((PresSensorValue - ConstantX) / Constant) + .1; // The .1 accounts for the sensor not sitting on the bottom
   //printf("PresSensorValue: %f, ConstantX: %f, Constant: %f\n", PresSensorValue, ConstantX, Constant );
   waterHeight = ((PresSensorValue*ConstantX) - Constant) + .33; 
   //printf("Water Height = %f\n", waterHeight);
   
   return waterHeight;

}

// Update the Kalman filter with a new sensor measurement
void updateKalmanFilter(KalmanFilter* filter, double measurement, double measurementError) {
    // Update the estimate based on the measurement and its error
    filter->value = filter->value + filter->gain * (measurement - filter->value);

    // Update the estimation error
    filter->error = (1.0 - filter->gain) * filter->error + filter->gain * measurementError;

    // Calculate the new Kalman gain for the next iteration
    filter->gain = filter->error / (filter->error + measurementError);
}

void MyMQTTSetup(char* mqtt_address){

   int rc;

   if ((rc = MQTTClient_create(&client, mqtt_address, TANKMON_CLIENTID,
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
   
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", TANKSENS_TOPICID, TANKSENS_CLIENTID, QOS);
   log_message("TankMonitor: Subscribing to topic: %s for client: %s\n", TANKSENS_TOPICID, TANKSENS_CLIENTID);
   MQTTClient_subscribe(client, TANKSENS_TOPICID, QOS);

   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", WELLMON_TOPICID, WELLMON_CLIENTID,  QOS);
   log_message("TankMonitor: Subscribing to topic: %s for client: %s\n", WELLMON_TOPICID, WELLMON_CLIENTID );
   MQTTClient_subscribe(client, WELLMON_TOPICID, QOS);
}
void MyMQTTPublish() {
   int rc;
   int i;
   time_t t;
   time(&t);

   if (verbose) {
      for (i=0; i<=TANKMON_LEN-1; i++) {
         printf("%f ", tankMon_.data_payload[i]);
      }
      printf("%s", ctime(&t));
   }
   pubmsg.payload = tankMon_.data_payload;
   pubmsg.payloadlen = TANKMON_LEN * 4;
   pubmsg.qos = QOS;
   pubmsg.retained = 0;
   deliveredtoken = 0;
   if ((rc = MQTTClient_publishMessage(client, TANKMON_TOPICID, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to publish message, return code %d\n", rc);
      log_message("TankMonitor: Error == Failed to Publish Message. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
   }


json_object *root = json_object_new_object();
for (i=0; i<=TANKMON_LEN-1; i++) {
   json_object_object_add(root, tankmon_ClientData_var_name [i], json_object_new_double(tankMon_.data_payload[i]));
}

const char *json_string = json_object_to_json_string(root);

pubmsg.payload = (void *)json_string; // Make sure to cast the const pointer to void pointer
pubmsg.payloadlen = strlen(json_string);
pubmsg.qos = QOS;
pubmsg.retained = 0;
MQTTClient_publishMessage(client, TANKMON_JSONID, &pubmsg, &token);
//printf("Waiting for publication of %s\non topic %s for client with ClientID: %s\n", json_string, TANK_TOPIC, TANK_MONID);
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
