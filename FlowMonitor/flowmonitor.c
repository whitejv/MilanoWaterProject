#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"


unsigned short int flow_data_payload[21];
#define CLIENTID     "ESP8266 ClientFlow"
#define TOPIC   "Flow ESP"
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

#define datafile "./datafile1.txt"
#define flowdata "./flowdata.txt"

float TotalDailyGallons = 0;
float TotalGPM = 0;

 
MQTTClient_deliveryToken deliveredtoken;
 
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    //printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}
 
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    time_t t;
    time(&t);
    int i;
    unsigned short int* payloadptr;
      
    printf("Message arrived:\n");
    printf("          topic: %s  ", topicName);
    printf("         length: %d  ", topicLen);
    printf("     PayloadLen: %d\n", message->payloadlen);
    printf("message: ");
    
   
    if (message->payloadlen != 0) {
         if ( message->payloadlen == 42) {
            payloadptr = (unsigned short int *)message->payload;
            for(i=0; i < (message->payloadlen/2); i++)
            {
               flow_data_payload[i] = *payloadptr++ ;
               printf("%0x ", flow_data_payload[i]);
             }
         }
         //printf("%0X ", raw_data_payload[21]);
         printf("%s", ctime(&t));
         //printf(".");
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
    }
    return 1;
}
 
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
    struct tm timenow;
    time(&t);
    int SecondsFromMidnight = 0 ;
    int PriorSecondsFromMidnight =0;
    float irrigationPressure = 0;
    float temperatureF;
    float calibrationFactor = 1.0;
    float flowRate = 0.0;
    float dailyGallons = 0;
    int pulseCount = 0;
    int millsElapsed = 0;
    int millsTotal = 0;
    int dailyPulseCount = 0;
    int newPulseData = 0;
    
    
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    
    if ((rc = MQTTClient_create(&client, ADDRESS, FL_CLIENTID,
                                MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        rc = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }
    
    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
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
        rc = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", TOPIC, CLIENTID, QOS);
    
    MQTTClient_subscribe(client, TOPIC, QOS);
    
    /*
     * Initialize the data file with headers
     */
     fptr = fopen(flowdata, "a");
     fprintf(fptr, "Hello World ");
     fclose(fptr);

    /*
     * Main Loop
     */
    
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
            
            fptr = fopen(datafile, "a");
            
            /* reset 24 hr stuff */
            
            fprintf(fptr, "%s", ctime(&t));
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
            
            pulseCount = flow_data_payload[0];
            dailyPulseCount = dailyPulseCount + pulseCount ;
            millsElapsed = flow_data_payload[1] ;
            millsTotal = millsTotal + millsElapsed;
            flowRate = ((pulseCount / (millsElapsed/1000)) / .5) / calibrationFactor;
            flowRate = ((flowRate * .00026417)/(millsElapsed/1000)) * 60;  //GPM
            printf("Pulse Count %d Daily Pulse Count %d Flow Rate: %f\n", pulseCount, dailyPulseCount, flowRate);
            dailyGallons = (dailyPulseCount/(millsTotal/1000)) / calibrationFactor * .00026417;
            
        } else {
            pulseCount = 0;
            millsElapsed = 0 ;
        }
        
        
        irrigationPressure = flow_data_payload[3] * .0015259021 ;
        
        temperatureF = *((float *)&flow_data_payload[17]);
        
        
        /*
         * Set Firmware Version
         * firmware = formatted_sensor_payload[20] & SubFirmware;
         */
  
        /*
         * Load Up the Data
         */
        
        /* CLIENTID     "Tank Subscriber", TOPIC "flow Data", flow_sensor_ */
        flow_sensor_payload[0] =    flowRate;
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

        for (i=0; i<=FL_LEN; i++) {
            printf("%f ", flow_sensor_payload[i]);
        }
        printf("%s", ctime(&t));

        pubmsg.payload = flow_sensor_payload;
        pubmsg.payloadlen = FL_LEN * 4;
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        deliveredtoken = 0;
        if ((rc = MQTTClient_publishMessage(client, FL_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
        {
            printf("Failed to publish message, return code %d\n", rc);
            rc = EXIT_FAILURE;
        }
        
        /*
         * Run at this interval
         */
        
        sleep(1) ;
    }
    
    MQTTClient_unsubscribe(client, F_TOPIC);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
