#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"


 
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    time_t t;
    time(&t);
    int i;
    unsigned short int* payloadptr;
    float* sensorpayloadptr;

      
    //printf("Message arrived:\n");
    //printf("          topic: %s  ", topicName);
    //printf("         length: %d  ", topicLen);
    //printf("     PayloadLen: %d\n", message->payloadlen);
    //printf("message: ");
   
   
    if (message->payloadlen != 0) {
         if ( message->payloadlen == 42) {
            payloadptr = (unsigned short int *)message->payload;
            for(i=0; i < (message->payloadlen/2); i++)
            {
               flow_data_payload[i] = *payloadptr++ ;
               //printf("%0x ", flow_data_payload[i]);
             }
             printf("|\n");
         }
         else if ( F_LEN*4 == message->payloadlen) {
            sensorpayloadptr = (float *)message->payload;
            for(i=0; i < (message->payloadlen/4); i++)
            {
               formatted_sensor_payload[i] = *sensorpayloadptr++ ;
               //printf("%0f ", formatted_sensor_payload[i]);
             }
             printf("+\n");
         }
         //printf("%s", ctime(&t));
         
         
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
    }
    return 1;
}
 

