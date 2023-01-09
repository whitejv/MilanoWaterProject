/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"
*/
/*
 * Cut and Paste Begins Here
 */

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
   
   int i;
   /*
   printf("Message arrived:\n");
   printf("          topic: %s  ", topicName);
   printf("         length: %d  ", topicLen);
   printf("     PayloadLen: %d\n", message->payloadlen);
   printf("message: ");
   */
   if ( strcmp(topicName, FLO_TOPIC) == 0) {
      memcpy(flow_data_payload, message->payload, message->payloadlen);
      //for(i=0; i < FLO_LEN; i++) {printf("%0x ", flow_data_payload[i]);}
      printf("|\n");
   }
   else if ( strcmp(topicName, F_TOPIC) == 0) {
      memcpy(formatted_sensor_payload, message->payload, message->payloadlen);
      //for(i=0; i < F_LEN; i++) { printf("%0f ", formatted_sensor_payload[i]);}
      printf("+\n");
   }
   else if ( strcmp(topicName, M_TOPIC) == 0) {
      memcpy(monitor_sensor_payload, message->payload, message->payloadlen);
      //for(i=0; i < M_LEN; i++) { printf("%0x ", monitor_sensor_payload[i]);}
      printf(".\n");
   }
   else if ( strcmp(topicName, A_TOPIC) == 0) {
      memcpy(alert_sensor_payload, message->payload, message->payloadlen);
      //for(i=0; i < A_LEN; i++) {printf("%0x ", alert_sensor_payload[i]);}
      printf("*\n");
   }
   else if ( strcmp(topicName, FL_TOPIC) == 0) {
      memcpy(flow_sensor_payload, message->payload, message->payloadlen);
      //for(i=0; i < FL_LEN; i++) {printf("%0f ", flow_sensor_payload[i]);}
      printf("^\n");
   }
   else if ( strcmp(topicName, ESP_TOPIC) == 0) {
      memcpy(data_payload, message->payload, message->payloadlen);
      //for(i=0; i < ESP_LEN; i++) {printf("%0x ", data_payload[i]);}
      printf("-\n");
   }
   
   MQTTClient_freeMessage(&message);
   MQTTClient_free(topicName);
   return 1;
}
       
       
