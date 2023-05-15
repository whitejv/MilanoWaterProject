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

int msgarrvdAsync(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
   
   int i;
   /*
   printf("Message arrived:\n");
   printf("          topic: %s  ", topicName);
   printf("         length: %d  ", topicLen);
   printf("     PayloadLen: %d\n", message->payloadlen);
   printf("message: ");
   */
   if ( strcmp(topicName, TANK_CLIENT) == 0) {
      memcpy(tank_data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < TANK_LEN; i++) {printf("%0x ", tank_data_payload[i]);}}
      printf("|\n");
   }
   else if ( strcmp(topicName, WELL_CLIENT) == 0) {
      memcpy(well_data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < WELL_LEN; i++) {printf("%0x ", well_data_payload[i]);}}
      printf("-\n");
   }
   else if ( strcmp(topicName, FLOW_CLIENT) == 0) {
      memcpy(flow_data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < FLOW_LEN; i++) {printf("%0x ", flow_data_payload[i]);}}
      printf(":\n");
   }
   else if ( strcmp(topicName, TANK_TOPIC) == 0) {
      memcpy(tank_sensor_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < TANK_DATA; i++) { printf("%0f ", tank_sensor_payload[i]);}}
      printf("+\n");
   }
   else if ( strcmp(topicName, WELL_TOPIC) == 0) {
      memcpy(well_sensor_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < WELL_DATA; i++) {printf("%0x ", well_sensor_payload[i]);}}
      printf(">\n");
   } 
   else if ( strcmp(topicName, FLOW_TOPIC) == 0) {
      memcpy(flow_sensor_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < FLOW_DATA; i++) {printf("%0f ", flow_sensor_payload[i]);}}
      printf("^\n");
   }
   else if ( strcmp(topicName, M_TOPIC) == 0) {
      memcpy(monitor_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < M_LEN; i++) { printf("%0x ", monitor_payload[i]);}}
      printf(".\n");
   }
   else if ( strcmp(topicName, A_TOPIC) == 0) {
      memcpy(alert_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < A_LEN; i++) {printf("%0x ", alert_payload[i]);}}
      printf("*\n");
   }

   MQTTAsync_freeMessage(&message);
   MQTTAsync_free(topicName);
   return 1;
}
       
       
