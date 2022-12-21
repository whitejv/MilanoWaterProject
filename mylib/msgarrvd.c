#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

/*
 * Cut and Paste Begins Here
 */

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
   
   int i;
   unsigned short int* payloadptr;
   float* sensorpayloadptr;
   
   
   printf("Message arrived:\n");
   printf("          topic: %s  ", topicName);
   printf("         length: %d  ", topicLen);
   printf("     PayloadLen: %d\n", message->payloadlen);
   printf("message: ");
   
   if ( strcmp(topicName, FLO_TOPIC) {
      memcpy(flow_data_payload, message->payload, topicLen)
      for(i=0; i < (message->payloadlen/2); i++)
      {
         printf("%0x ", flow_data_payload[i]);
      }
      printf("|\n");
      
   }
   else if ( strcmp(topicName, F_TOPIC)) {
      memcpy(formatted_sensor_payload, message->payload, topicLen)
      for(i=0; i < (message->payloadlen/4); i++)
      {
         printf("%0f ", formatted_sensor_payload[i]);
      }
      printf("+\n");
   }
       
       
       MQTTClient_freeMessage(&message);
       MQTTClient_free(topicName);
       return 1;
}
       
       
