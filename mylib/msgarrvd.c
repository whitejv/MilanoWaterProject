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
   else if ( strcmp(topicName, "rainbird/controller1/active_zone") == 0) {
      memcpy(rainbird_payload, message->payload, message->payloadlen);
      if (verbose) {printf("%s ", rainbird_payload);}
      
      /* Process rainbird zone data */
      
      parse_message(rainbird_payload, &Front_Controller);

      if (verbose){printf("Front Controller: %d\n", Front_Controller.controller);}
      if (verbose){for (int i = 0; i < MAX_ZONES; i++) {printf("Front Zone: %d, Status: %d\n", Front_Controller.states[i].station, Front_Controller.states[i].status);}}
      
      printf("F\n");
   }
      else if ( strcmp(topicName, "rainbird/controller2/active_zone") == 0) {
      memcpy(rainbird_payload, message->payload, message->payloadlen);
      if (verbose) {printf("%s ", rainbird_payload);}
      
      /* Process rainbird zone data */
      
      parse_message(rainbird_payload, &Back_Controller);

      if (verbose){printf("Back Controller: %d\n", Back_Controller.controller);}
      if (verbose){for (int i = 0; i < MAX_ZONES; i++) {printf("Back Zone: %d, Status: %d\n", Back_Controller.states[i].station, Back_Controller.states[i].status);}}
      
      printf("F\n");
   }


   MQTTClient_freeMessage(&message);
   MQTTClient_free(topicName);
   return 1;
}
 void parse_message(char* message, Controller* controller ) {
    char* token;
    int   station;
    int   status;
    int   controller_num;

    // Parse controller number
    sscanf(message, "Controller Number %d", &(controller_num));
    
   controller->controller = controller_num;
   // Find start of states
   token = strstr(message, "states");
   token += strlen("states: ");

   // Parse states
   int i = 0;
   while ((token = strtok(token, ", ")) != NULL) {
      sscanf(token, "%d:%d", &station, &status);
      controller->states[i].station = station;
      controller->states[i].status = status;
      i++;
      token = NULL;
   }
}      
       
