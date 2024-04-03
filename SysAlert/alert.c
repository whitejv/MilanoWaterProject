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
#include <stdbool.h> // Required for using 'bool' type

typedef enum {
    CONDITION_EQUAL,
    CONDITION_NOT_EQUAL,
    CONDITION_GREATER_THAN,
    CONDITION_LESS_THAN,
    // Add more conditions as needed
} TriggerCondition;

int verbose = FALSE;
enum AlarmInternalState
{
   inactive = 0,
   trigger = 1,
   active = 2,
   reset = 3,
   timeout = 4
};

typedef struct {
    int alarmState;      // Alarm state
    int alarmStatePrior; // Last Alarm State
    int internalState;   // field for internal state
    int timer;           // field for timer
    int timeOut;         // field for timeOut
    int triggerDelay ;   // field for triggerDelay
    int occurencesHour ;
    int occurencesDay  ;
    const int TIMEOUT_RESET_VALUE ;
    const int TIMER_INCREMENT ;
    const int TRIGGER_VALUE ;
    const int TRIGGER_DELAY ;
} AlarmStructure;

#define ALARM_COUNT 20
AlarmStructure alarms[ALARM_COUNT+1] = {0, inactive, 0, 10, 1, 10, 1, 5,
                                        0, inactive, 0, 10, 1, 10, 1, 5,
                                        0, inactive, 0, 10, 1, 10, 1, 5,
                                        0, inactive, 0, 10, 1, 10, 1, 5,
                                        0, inactive, 0, 10, 1, 10, 1, 5,
                                        0, inactive, 0, 10, 1, 10, 1, 30,
                                        0, inactive, 0, 10, 1, 10, 1, 30,
                                        0, inactive, 0, 10, 1, 10, 1, 5,
                                        0, inactive, 0, 10, 1, 10, 1, 5,
                                        0, inactive, 0, 10, 1, 10, 1, 5};

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

   printf("Subscribing to all monitor topics: %s\nfor client: %s using QoS: %d\n\n", "mwp/data/monitor/#", IRRIGATIONMON_CLIENTID, QOS);
   log_message("Alert: Subscribing to topic: %s for client: %s\n", "mwp/data/monitor/#", IRRIGATIONMON_CLIENTID);
   MQTTClient_subscribe(client, "mwp/data/monitor/#", QOS);

   /*
    * Main Loop
    */

   log_message("Alert: Entering Main Loop\n");

   while (1)
   {
      /*
      printf("Step: %d\n", stepcount);  
      switch (stepcount)
      {
         case 5:
            printf("trigger case\n");
            wellMon_.well.House_tank_pressure_switch_on = ON;
            wellMon_.well.well_pump_1_on = OFF;
            wellMon_.well.well_pump_2_on = OFF;
            tankMoney = ON ;
            break;
         case 16:
            printf("inactive case\n");
            wellMon_.well.House_tank_pressure_switch_on = OFF;
            wellMon_.well.well_pump_1_on = ON;
            //wellMon_.well.well_pump_2_on = OFF;
            tankMoney = OFF ;
            break;
         
      }
      stepcount = stepcount + 1;
      */

      alarmSummary = 0;

      /**	1 -	Critical	Tank Critically Low	*/
      //printf("Tank Float 2: %d\n", tankMoney);
      processAlarmGeneric(&alarms[1], CONDITION_EQUAL, tankMon_.tank.float2, ON);
      /**	2 - Critical	Irrigation Pump Temp Low	*/
      //printf("irrigation pump temp: %d\n", irrigationMon_.irrigation.PumpTemp);
      processAlarmGeneric(&alarms[2], CONDITION_LESS_THAN, irrigationMon_.irrigation.PumpTemp, 35);
      /**	3 -	Critical	House Water Pressure Low	*/
      //printf("house water pressure: %d\n", houseMon_.house.housePressure);
      processAlarmGeneric(&alarms[3], CONDITION_LESS_THAN, houseMon_.house.housePressure, 25);
      /**	4 -	Critical	Septic System Alert	*/
      //printf("septic alert: %d\n", wellMon_.well.septic_alert_on);
      processAlarmGeneric(&alarms[4], CONDITION_EQUAL, wellMon_.well.septic_alert_on, ON);
      /**	5 - Critical	Irrigation Pump Run Away	*/
      //printf("irrigation pump run away: %d\n", tankMoney);
      //processAlarmGeneric(&alarms[5], CONDITION_LESS_THAN, irrigationMon_.irrigation.FlowPerMin, 2);
      /**	6 -	Critical	Well 3 Pump Run Away	*/
      //printf("well pump run away: %d\n", tankMoney);
      //processAlarmGeneric(&alarms[6], CONDITION_LESS_THAN, tankMon_.tank.tank_gallons_per_minute, 2);
      /**	7 -	Warn	Well Pumps Not Starting	*/
      processAlarmWellPumpsNotStarting(&alarms[7]);
      /**	7 - 	Warn	Well Pumps Runtime Exceeded	*/

      /**	8  -	Warn	Well Pumps Cycles Excessive	*/

      /**	9 -	Info	Well Protect Circuit Active	*/

      /**	10 - 	Warn	Tank Overfill Condition	*/
      //printf("tank overfill: %d\n", tankMon_.tank.tank_gallons);
      processAlarmGeneric(&alarms[10], CONDITION_GREATER_THAN, tankMon_.tank.tank_gallons, 2300);
      
      /*
       * Load Up the Data
       */
      alert_.alert.alert1 = ((uint32_t)alarms[1].internalState << 16) | alarms[1].alarmState;
      alert_.alert.alert2 = ((uint32_t)alarms[2].internalState << 16) | alarms[2].alarmState;
      alert_.alert.alert3 = ((uint32_t)alarms[3].internalState << 16) | alarms[3].alarmState;
      alert_.alert.alert4 = ((uint32_t)alarms[4].internalState << 16) | alarms[4].alarmState;
      alert_.alert.alert5 = ((uint32_t)alarms[5].internalState << 16) | alarms[5].alarmState;
      alert_.alert.alert6 = ((uint32_t)alarms[6].internalState << 16) | alarms[6].alarmState;
      alert_.alert.alert7 = ((uint32_t)alarms[7].internalState << 16) | alarms[7].alarmState;
      alert_.alert.alert8 = ((uint32_t)alarms[8].internalState << 16) | alarms[8].alarmState;
      alert_.alert.alert9 = ((uint32_t)alarms[9].internalState << 16) | alarms[9].alarmState;
      alert_.alert.alert10 = ((uint32_t)alarms[10].internalState << 16) | alarms[10].alarmState;
      alert_.alert.alert11 = ((uint32_t)alarms[11].internalState << 16) | alarms[11].alarmState;
      alert_.alert.alert12 = ((uint32_t)alarms[12].internalState << 16) | alarms[12].alarmState;
      alert_.alert.alert13 = ((uint32_t)alarms[13].internalState << 16) | alarms[13].alarmState;
      alert_.alert.alert14 = 0;
      alert_.alert.alert15 = 0;
      alert_.alert.alert16 = 0;
      alert_.alert.alert17 = 0;
      alert_.alert.alert18 = 0;
      alert_.alert.alert19= 0;
      alert_.alert.alert20 = 0;

      /*
       * Determine if an Alarm was detected
       */
      alarmSummary = 0;
      for (i = 0; i < ALARM_COUNT; i++) {
         if (alarms[i].alarmState == 1) {
            alarmSummary = 1;
         }
      }

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

   char *alarm_names[21] = {"unused",
                            "Low Water Alert",
                            "Irrigation Pump Temp",
                            "House Water Pressure Low",
                            "Septic System Alert",
                            "Irrigation Pump Run Away",
                            "Well 3 Pump Run Away",
                            "Well Pumps Not Starting",
                            "Well Pump Runtime Excessive",
                            "Well Pumps Cycles Excessive",
                            "Well Protect Circuit Active",
                            "Tank Overfill Condition",
                            "I2C Faults Detected",
                            "Zone Use Excessive",
                            "undefined",
                            "undefined",
                            "undefined",
                            "undefined",
                            "undefined",
                            "undefined"};

   if (verbose) {
      for (i = 0; i <= ALERT_LEN; i++) {
         printf("%x ", alert_.data_payload[i]);
      }
      printf("%s", ctime(&t));
   }

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
   
   //if (alarmSummary == 1) {
      json_object *root = json_object_new_object();

      for (i = 0; i < ALARM_COUNT; i++) {
         json_object *alarm_obj = json_object_new_object();
         json_object_object_add(alarm_obj, "alarm_state", json_object_new_int(alarms[i].alarmState));
         json_object_object_add(alarm_obj, "internal_state", json_object_new_int(alarms[i].internalState));
         json_object_object_add(root, alarm_names[i], alarm_obj);
      }
   
      const char *json_string = json_object_to_json_string(root);

      pubmsg.payload = (void *)json_string;
      pubmsg.payloadlen = strlen(json_string);
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      MQTTClient_publishMessage(client, ALERT_JSONID, &pubmsg, &token);
      MQTTClient_waitForCompletion(client, token, TIMEOUT);

      json_object_put(root); // Free the memory allocated to the JSON object
   //}
}

// Refactor the function to improve readability
void processAlarmWellPumpsNotStarting(AlarmStructure* alarm) {
    
    if (alarm == NULL) return; // Add null pointer check for safety

    switch (alarm->internalState) {
        case timeout:
            if (alarm->timeOut == 0) {
                alarm->internalState = inactive;
            } else {
                alarm->timeOut -= alarm->TIMER_INCREMENT;
            }
            break;
        case inactive:
            if (wellMon_.well.House_tank_pressure_switch_on == ON) {
                alarm->internalState = trigger;
                alarm->timer = 0;
            }
            break;
        case trigger:
            if (alarm->timer >= alarm->TRIGGER_DELAY) {
                if (wellMon_.well.House_tank_pressure_switch_on == ON && 
                   (wellMon_.well.well_pump_1_on == OFF || wellMon_.well.well_pump_2_on == OFF)) {
                    alarm->internalState = active;
                    alarm->timer = 0;
                }
            } else {
                alarm->timer += alarm->TIMER_INCREMENT;
            }
            break;
        case active:
            if (wellMon_.well.House_tank_pressure_switch_on == ON && 
               (wellMon_.well.well_pump_1_on == OFF || wellMon_.well.well_pump_2_on == OFF)) {
                alarm->alarmState = 1;
                log_message("Well Pumps 1, 2 not starting.");
            } else {
                alarm->internalState = reset;
            }
            break;
        case reset:
            if (wellMon_.well.House_tank_pressure_switch_on == OFF) {
                alarm->alarmState = 0;
                alarm->timer = alarm->TRIGGER_DELAY;
                alarm->timeOut = alarm->TIMEOUT_RESET_VALUE;
                alarm->internalState = timeout;
            }
            break;
        default:
            // Optionally log an error or handle unexpected internalState
            break;
    }
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
            } else {
                alarm->timer += alarm->TIMER_INCREMENT;
            }
            break;

        case active:
            if (triggerConditionMet) {
                alarm->alarmState = 1; // Alarm triggered
                // Log message or perform additional actions
            } else {
                alarm->internalState = reset;
            }
            break;

        case reset:
            alarm->alarmState = 0;
            alarm->timer = alarm->TRIGGER_DELAY;
            alarm->timeOut = alarm->TIMEOUT_RESET_VALUE;
            alarm->internalState = timeout;
            break;

        default:
            // Error handling or logging for unexpected internalState
            break;
    }
}