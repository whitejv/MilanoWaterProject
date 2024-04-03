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
   
   if (verbose) {
      printf("Message arrived:\n");
      printf("          topic: %s  ", topicName);
      printf("         length: %d  ", topicLen);
      printf("     PayloadLen: %d\n", message->payloadlen);
      printf("message: ");
   }

   if ( strcmp(topicName, TANKSENS_TOPICID) == 0) {
      memcpy(tankSens_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < TANKSENS_LEN; i++) {printf("%0x ", tankSens_.data_payload[i]);}}
      printf("t\n");
   }
   else if ( strcmp(topicName, WELLSENS_TOPICID) == 0) {
      memcpy(wellSens_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < WELLSENS_LEN; i++) {printf("%0x ", wellSens_.data_payload[i]);}}
      printf("w\n");
   }
   else if ( strcmp(topicName, IRRIGATIONSENS_TOPICID) == 0) {
      memcpy(irrigationSens_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < IRRIGATIONSENS_LEN; i++) {printf("%0x ", irrigationSens_.data_payload[i]);}}
      printf("f\n");
   }
   else if ( strcmp(topicName, TANKMON_TOPICID) == 0) {
      memcpy(tankMon_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < TANKMON_LEN; i++) { printf("%0f ", tankMon_.data_payload[i]);}}
      printf("+\n");
   }
   else if ( strcmp(topicName, WELLMON_TOPICID) == 0) {
      memcpy(wellMon_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < WELLMON_LEN; i++) {printf("%0x ", wellMon_.data_payload[i]);}}
      printf(">\n");
   } 
   else if ( strcmp(topicName, IRRIGATIONMON_TOPICID) == 0) {
      memcpy(irrigationMon_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < IRRIGATIONMON_LEN; i++) {printf("%0f ", irrigationMon_.data_payload[i]);}}
      printf("^\n");
   }
   else if ( strcmp(topicName, MONITOR_TOPICID) == 0) {
      memcpy(monitor_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < MONITOR_LEN; i++) { printf("%0x ", monitor_.data_payload[i]);}}
      printf(".\n");
   }
   else if ( strcmp(topicName, ALERT_TOPICID) == 0) {
      memcpy(alert_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < ALERT_LEN; i++) {printf("%0x ", alert_.data_payload[i]);}}
      printf("*\n");
   }
   else if ( strcmp(topicName, "mwp/response/rainbird/controller1/active_zone") == 0) {
      memcpy(rainbird_payload, message->payload, message->payloadlen);
      if (verbose) {printf("%s ", rainbird_payload);}
      
      /* Process rainbird zone data */
      
      parse_message(rainbird_payload, &Front_Controller);

      if (verbose){printf("Front Controller: %d\n", Front_Controller.controller);}
      if (verbose){for (int i = 0; i < MAX_ZONES; i++) {printf("Front Zone: %d, Status: %d\n", Front_Controller.states[i].station, Front_Controller.states[i].status);}}
      
      printf("F\n");
   }
      else if ( strcmp(topicName, "mwp/response/rainbird/controller2/active_zone") == 0) {
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
       
