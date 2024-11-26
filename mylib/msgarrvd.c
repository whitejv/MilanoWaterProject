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
   time_t t;
   time(&t);
   int i;
   
   if (verbose) {
      printf("Message arrived: %s", ctime(&t));
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
   else if ( strcmp(topicName, WELL3SENS_TOPICID) == 0) {
       memcpy(well3Sens_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < WELL3SENS_LEN; i++) {printf("%0x ", well3Sens_.data_payload[i]);}}
      printf("w3\n");
   }
   else if ( strcmp(topicName, HOUSESENS_TOPICID) == 0) {
      memcpy(houseSens_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < HOUSESENS_LEN; i++) {printf("%0x ", houseSens_.data_payload[i]);}}
      printf("h\n");
   }
   else if ( strcmp(topicName, IRRIGATIONSENS_TOPICID) == 0) {
      memcpy(irrigationSens_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < IRRIGATIONSENS_LEN; i++) {printf("%0x ", irrigationSens_.data_payload[i]);}}
      printf("i\n");
   }
   else if ( strcmp(topicName, TANKMON_TOPICID) == 0) {
      memcpy(tankMon_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < TANKMON_LEN; i++) { printf("%0f ", tankMon_.data_payload[i]);}}
      printf("+\n");
   }
   else if ( strcmp(topicName, WELLMON_TOPICID) == 0) {
      memcpy(wellMon_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < WELLMON_LEN; i++) {printf("%0f ", wellMon_.data_payload[i]);}}
      printf(">\n");
   }
   else if ( strcmp(topicName, WELL3MON_TOPICID) == 0) {
      memcpy(well3Mon_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < WELL3MON_LEN; i++) {printf("%0f ", well3Mon_.data_payload[i]);}}
      printf(">\n");
   }
   else if ( strcmp(topicName, HOUSEMON_TOPICID) == 0) {
      memcpy(houseMon_.data_payload, message->payload, message->payloadlen);
      if (verbose) {for(i=0; i < HOUSEMON_LEN; i++) {printf("%0f ", houseMon_.data_payload[i]);}}
      printf("/\n");
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
      if (strcmp(rainbird_payload, "Irrigation stopped") == 0) {
         if (verbose) {printf("Front Controller: Irrigation Stopped.\n");}
      }
      else if (strcmp(rainbird_payload, "Zone Advanced") == 0) {
         if (verbose) {printf("Front Controller: Zone Advanced.\n");}
      }
      else {
         /* Process rainbird zone data */
         
         parse_message(rainbird_payload, &Front_Controller);

         if (verbose){printf("Front Controller: %d\n", Front_Controller.controller);}
         if (verbose){for (int i = 0; i < MAX_ZONES; i++) {printf("Front Zone: %d, Status: %d\n", Front_Controller.states[i].station, Front_Controller.states[i].status);}}
      }
      printf("F\n");
   }
      else if ( strcmp(topicName, "mwp/response/rainbird/controller2/active_zone") == 0) {
      memcpy(rainbird_payload, message->payload, message->payloadlen);
      if (verbose) {printf("%s ", rainbird_payload);}
           
      if (strcmp(rainbird_payload, "Irrigation stopped") == 0) {
         if (verbose) {printf("Back Controller: Irrigation Stopped.\n");}
      }
      else if (strcmp(rainbird_payload, "Zone Advanced") == 0) {
         if (verbose) {printf("Back Controller: Zone Advanced.\n");}
      }
      else {
         /* Process rainbird zone data */
         
         parse_message(rainbird_payload, &Back_Controller);

         if (verbose){printf("Back Controller: %d\n", Back_Controller.controller);}
         if (verbose){for (int i = 0; i < MAX_ZONES; i++) {printf("Back Zone: %d, Status: %d\n", Back_Controller.states[i].station, Back_Controller.states[i].status);}}
      }
      printf("B\n");
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
       
