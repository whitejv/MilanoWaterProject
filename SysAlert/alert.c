#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <json-c/json.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"
#include "../include/alert.h"
#include <stdbool.h> // Required for using 'bool' type

int verbose = FALSE;




AlarmStructure alarms[ALARM_COUNT+1] = {0};

int alarmSummary = 0;


MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;

void MyMQTTPublish(void) ;
void processAlarmWellPumpsNotStarting(AlarmStructure* p_alarm ) ;
void processAlarmTankCriticallyLow(AlarmStructure* p_alarm) ;

MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
   // printf("Message with token value %d delivery confirmed\n", dt);
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

int main(int argc, char *argv[])
{
   int i = 0;
   int j = 0;
   int stepcount = 0;
   time_t t;
   time(&t);

   log_message("Alert: Started\n");

   int rc;
   int opt;
   const char *mqtt_ip;
   int mqtt_port;

   while ((opt = getopt(argc, argv, "vPD")) != -1) {
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
         default:
               fprintf(stderr, "Usage: %s [-v] [-P | -D]\n", argv[0]);
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

   if ((rc = MQTTClient_create(&client, mqtt_address, ALERT_CLIENTID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      log_message("Alert: Error == Failed to Create Client. Return Code: %d\n", rc);
      printf("Failed to create client, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      log_message("Alert: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
      printf("Failed to set callbacks, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   conn_opts.keepAliveInterval = 20;
   conn_opts.cleansession = 1;
   // conn_opts.username = mqttUser;       //only if req'd by MQTT Server
   // conn_opts.password = mqttPassword;   //only if req'd by MQTT Server
   if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
   {
      log_message("Alert: Error == Failed to Connect. Return Code: %d\n", rc);
      printf("Failed to connect, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   printf("Subscribing to all monitor topics: %s\nfor client: %s using QoS: %d\n\n", "mwp/data/monitor/#", ALERT_CLIENTID, QOS);
   log_message("Alert: Subscribing to topic: %s for client: %s\n", "mwp/data/monitor/#", ALERT_CLIENTID);
   MQTTClient_subscribe(client, "mwp/data/monitor/#", QOS);


   /*
    * Main Loop
    */

   log_message("Alert: Entering Main Loop\n");

   while (1)
   {

      alarmSummary = 0;

      /**	1 -	Critical	Tank Critically Low	*/
      printf("%s %x\n", alarmInfo[1].label, tankMon_.tank.float2);
      processAlarmGeneric(&alarms[1], alarmInfo[1].conditional1, tankMon_.tank.float2, alarmInfo[1].trigValue1);
      /**	2 - Critical	Tank Overfill	*/
      printf("%s %f\n", alarmInfo[2].label,tankMon_.tank.tank_gallons);
      processAlarmGeneric(&alarms[2], alarmInfo[2].conditional1, (int)tankMon_.tank.tank_gallons, alarmInfo[2].trigValue1);
      /**	3 - Critical	Irrigation Pump Temp Low	*/
      printf("%s %f\n",  alarmInfo[3].label, irrigationMon_.irrigation.temperatureF);
      processAlarmGeneric(&alarms[3], alarmInfo[3].conditional1, (int)irrigationMon_.irrigation.temperatureF, alarmInfo[3].trigValue1);
      /**	4 - Critical	Irrigation Pump Temp Low	*/
      printf("%s %f\n", alarmInfo[4].label, houseMon_.house.temperatureF);
      processAlarmGeneric(&alarms[4], alarmInfo[4].conditional1, (int)houseMon_.house.temperatureF, alarmInfo[4].trigValue1);        
      /**	5 -	Critical	House Water Pressure Low	*/
      printf("%s %d\n", alarmInfo[5].label, houseMon_.house.pressurePSI);
      processAlarmGeneric(&alarms[5], alarmInfo[5].conditional1, (int)houseMon_.house.pressurePSI, alarmInfo[5].trigValue1);  

      
      /*
       * Load Up the Data
       */
      alert_.alert.Alert1 = alarms[1].internalState << 24 | alarms[1].occurences << 16 | alarms[1].eventSend << 8 | alarms[1].alarmState;
      alert_.alert.Alert2 = alarms[2].internalState << 24 | alarms[2].occurences << 16 | alarms[2].eventSend << 8 | alarms[2].alarmState;
      alert_.alert.Alert3 = alarms[3].internalState << 24 | alarms[3].occurences << 16 | alarms[3].eventSend << 8 | alarms[3].alarmState;
      alert_.alert.Alert4 = alarms[4].internalState << 24 | alarms[4].occurences << 16 | alarms[4].eventSend << 8 | alarms[4].alarmState;
      alert_.alert.Alert5 = alarms[5].internalState << 24 | alarms[5].occurences << 16 | alarms[5].eventSend << 8 | alarms[5].alarmState;
      alert_.alert.Alert6 = alarms[6].internalState << 24 | alarms[6].occurences << 16 | alarms[6].eventSend << 8 | alarms[6].alarmState;
      alert_.alert.Alert7 = alarms[7].internalState << 24 | alarms[7].occurences << 16 | alarms[7].eventSend << 8 | alarms[7].alarmState;
      alert_.alert.Alert8 = alarms[8].internalState << 24 | alarms[8].occurences << 16 | alarms[8].eventSend << 8 | alarms[8].alarmState;
      alert_.alert.Alert9 = alarms[9].internalState << 24 | alarms[9].occurences << 16 | alarms[9].eventSend << 8 | alarms[9].alarmState;
      alert_.alert.Alert10 = alarms[10].internalState << 24 | alarms[10].occurences << 16 | alarms[10].eventSend << 8 | alarms[10].alarmState;
      alert_.alert.Alert11 = alarms[11].internalState << 24 | alarms[11].occurences << 16 | alarms[11].eventSend << 8 | alarms[11].alarmState;
      alert_.alert.Alert12 = alarms[12].internalState << 24 | alarms[12].occurences << 16 | alarms[12].eventSend << 8 | alarms[12].alarmState;
      alert_.alert.Alert13 = alarms[13].internalState << 24 | alarms[13].occurences << 16 | alarms[13].eventSend << 8 | alarms[13].alarmState;
      alert_.alert.Alert14 = alarms[14].internalState << 24 | alarms[14].occurences << 16 | alarms[14].eventSend << 8 | alarms[14].alarmState;
      alert_.alert.Alert15 = alarms[15].internalState << 24 | alarms[15].occurences << 16 | alarms[15].eventSend << 8 | alarms[15].alarmState;
      alert_.alert.Alert16 = alarms[16].internalState << 24 | alarms[16].occurences << 16 | alarms[16].eventSend << 8 | alarms[16].alarmState;
      alert_.alert.Alert17 = alarms[17].internalState << 24 | alarms[17].occurences << 16 | alarms[17].eventSend << 8 | alarms[17].alarmState;
      alert_.alert.Alert18 = alarms[18].internalState << 24 | alarms[18].occurences << 16 | alarms[18].eventSend << 8 | alarms[18].alarmState;
      alert_.alert.Alert19 = alarms[19].internalState << 24 | alarms[19].occurences << 16 | alarms[19].eventSend << 8 | alarms[19].alarmState;
      if (verbose) {
        for (i = 0; i <= ALERT_LEN; i++) {
            printf("%x ", alert_.data_payload[i]);
        }
        printf("%s", ctime(&t));
      }

      /*
       * Determine if an Alarm was detected
       */
      alarmSummary = 1;
     /* for (i = 0; i < ALARM_COUNT; i++) {
         if (alarms[i].alarmState == 1) {
            alarmSummary = 1;
         }
      }
     */
      MyMQTTPublish() ;

      /*
       * Run at this interval
       */

      sleep(1);
   }
   log_message("Alert: Exiting Main Loop\n");
   MQTTClient_unsubscribe(client, "mwp/data/monitor/#");
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
void MyMQTTPublish() {
   int rc;
   int i;
   time_t t;
   time(&t);

   pubmsg.payload = alert_.data_payload;
   pubmsg.payloadlen = ALERT_LEN * 4;
   pubmsg.qos = QOS;
   pubmsg.retained = 0;
   deliveredtoken = 0;

   if ((rc = MQTTClient_publishMessage(client, ALERT_TOPICID, &pubmsg, &token)) != MQTTCLIENT_SUCCESS) {
      log_message("Alert: Error == Failed to Publish Message. Return Code: %d\n", rc);
      printf("Failed to publish message, return code %d\n", rc);
      rc = EXIT_FAILURE;
   }
   /* 
    * Publish JSON Message Only if an Alarm is active 
    */
   

      json_object *root = json_object_new_object();

    for (i = 0; i < ALARM_COUNT; i++) {
        json_object *alarm_obj = json_object_new_object();
        json_object_object_add(alarm_obj, "alarm_state", json_object_new_int(alarms[i].alarmState));
        json_object_object_add(alarm_obj, "internal_state", json_object_new_int(alarms[i].internalState));
        json_object_object_add(alarm_obj, "event_send", json_object_new_int(alarms[i].eventSend));
        json_object_object_add(alarm_obj, "occurences", json_object_new_int(alarms[i].occurences));
        json_object_object_add(root, alarmInfo[i].label, alarm_obj);
    }
    

    const char *json_string = json_object_to_json_string(root);
    pubmsg.payload = (void *)json_string;
    pubmsg.payloadlen = strlen(json_string);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, ALERT_JSONID, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, TIMEOUT);

    json_object_put(root); // Free the memory allocated to the JSON object
         
}

void processAlarmGeneric(AlarmStructure* alarm, TriggerCondition condition, int triggerValue, int comparisonValue) {
    if (alarm == NULL) return;

    bool triggerConditionMet = false;

    // Determine if the trigger condition is met
    switch (condition) {
        case CONDITION_EQUAL:
            triggerConditionMet = (triggerValue == comparisonValue);
            
            break;
        case CONDITION_NOT_EQUAL:
            triggerConditionMet = (triggerValue != comparisonValue);
            
            break;
        case CONDITION_GREATER_THAN:
            triggerConditionMet = (triggerValue > comparisonValue);
            break;
        case CONDITION_LESS_THAN:
            triggerConditionMet = (triggerValue < comparisonValue);
            
            break;
        // Add more conditions as needed
    }

    // State machine logic
    switch (alarm->internalState) {
        case timeout:
            if (alarm->timeOut == 0) {
                alarm->internalState = inactive;
            } else {
                alarm->timeOut -= alarm->TIMER_INCREMENT;
            }
            break;

        case inactive:
            if (triggerConditionMet) {
                alarm->internalState = trigger;
                alarm->timer = alarm->TRIGGER_DELAY;
            }
            break;

        case trigger:
            if (alarm->timer >= alarm->TRIGGER_DELAY) {
                alarm->internalState = active;
                alarm->timer = 0;
                alarm->occurences++;
            } else {
                alarm->timer += alarm->TIMER_INCREMENT;
            }
            break;

        case active:
            if (triggerConditionMet) {
                alarm->alarmState = active; // Alarm triggered
                // Log message or perform additional actions
                if (alarm->eventSend == FALSE && alarm->eventSent == FALSE) {
                   alarm->eventSend = TRUE;
                   alarm->eventSent = TRUE;
                }
                else if (alarm->eventSend == TRUE && alarm->eventSent == TRUE) {
                   alarm->eventSend = FALSE;
                }
            } else {
                alarm->internalState = reset;
            }
            break;

        case reset:
            alarm->alarmState = 0;
            alarm->timer = alarm->TRIGGER_DELAY;
            alarm->timeOut = alarm->TIMEOUT_RESET_VALUE;
            alarm->internalState = timeout;
            alarm->eventSend = FALSE;
            break;

        default:
            // Error handling or logging for unexpected internalState
            break;
    }
}