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

int verbose = FALSE;

typedef struct {
    uint16_t alarmState;     // 16-bit field for Alarm state
    uint16_t internalState;  // 16-bit field for internal state
    uint16_t timer;          // 16-bit field for timer
    uint16_t timeOut;        // 16-bit field for timeOut
} AlarmStructure;

#define ALARM_COUNT 20
AlarmStructure alarms[ALARM_COUNT];
struct AlertData AlertData;

int alarmSummary = 0;
enum AlarmInternalState
{
   inactive = 0,
   trigger = 1,
   active = 2,
   reset = 3,
   timeout = 4
};

struct WellMonitorData WellSensorData;
struct TankMonitorData TankSensorData;
struct FlowMonitorData FlowSensorData;

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;

void MyMQTTPublish(void) ;
void processAlarmWellPumpsNotStarting(void) ;
void processAlarmTankCriticallyLow(void) ;

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

   if ((rc = MQTTClient_create(&client, mqtt_address, ALERT_ID,
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
   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", WELL_TOPIC, ALERT_ID, QOS);
   log_message("Alert: Subscribing to topic: %s for client: %s\n", WELL_TOPIC, ALERT_ID);
   MQTTClient_subscribe(client, WELL_TOPIC, QOS);

   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", TANK_TOPIC, ALERT_ID, QOS);
   log_message("Alert: Subscribing to topic: %s for client: %s\n", TANK_TOPIC, ALERT_ID);
   MQTTClient_subscribe(client, TANK_TOPIC, QOS);

   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", FLOW_TOPIC, ALERT_ID, QOS);
   log_message("Alert: Subscribing to topic: %s for client: %s\n", FLOW_TOPIC, ALERT_ID);
   MQTTClient_subscribe(client, FLOW_TOPIC, QOS);


   /*
    * Main Loop
    */

   log_message("Alert: Entering Main Loop\n");

   while (1)
   {


      /*
       *  Populate the structure with the sensor array data
       */

      memcpy((void *)&WellSensorData, (void *)well_sensor_payload, sizeof(struct WellMonitorData));
      memcpy((void *)&TankSensorData, (void *)tank_sensor_payload, sizeof(struct TankMonitorData));
      memcpy((void *)&FlowSensorData, (void *)flow_sensor_payload, sizeof(struct FlowMonitorData));
      
      alarmSummary = 0;

/**	1 -	Critical	Tank Critically Low	*/
      processAlarmTankCriticallyLow();
    
      /**	2- 	Critical	Irrigation Pump Temp Low/High	*/

      /**	3 -	Critical	House Water Pressure Low	*/

      /**	4 -	Critical	Septic System Alert	*/

      /**	5 - 	Critical	Irrigation Pump Run Away	*/

      /**	6 -	Critical	Well 3 Pump Run Away	*/

/**	7 -	Warn	Well Pumps Not Starting	*/
      processAlarmWellPumpsNotStarting();


      /**	8 - 	Warn	Well Pumps Runtime Exceeded	*/

      /**	9  -	Warn	Well Pumps Cycles Excessive	*/

      /**	10 -	Info	Well Protect Circuit Active	*/

      /**	11 - 	Warn	Tank Overfill Condition	*/

      /*
       * Load Up the Data
       */

      AlertData.alarm1 = ((uint32_t)alarms[1].internalState << 16) | alarms[1].alarmState;
      AlertData.alarm2 = ((uint32_t)alarms[2].internalState << 16) | alarms[2].alarmState;
      AlertData.alarm3 = ((uint32_t)alarms[3].internalState << 16) | alarms[3].alarmState;
      AlertData.alarm4 = ((uint32_t)alarms[4].internalState << 16) | alarms[4].alarmState;
      AlertData.alarm5 = ((uint32_t)alarms[5].internalState << 16) | alarms[5].alarmState;
      AlertData.alarm6 = ((uint32_t)alarms[6].internalState << 16) | alarms[6].alarmState;
      AlertData.alarm7 = ((uint32_t)alarms[7].internalState << 16) | alarms[7].alarmState;
      AlertData.alarm8 = ((uint32_t)alarms[8].internalState << 16) | alarms[8].alarmState;
      AlertData.alarm9 = ((uint32_t)alarms[9].internalState << 16) | alarms[9].alarmState;
      AlertData.alarm10 = ((uint32_t)alarms[10].internalState << 16) | alarms[10].alarmState;
      AlertData.alarm11 = ((uint32_t)alarms[11].internalState << 16) | alarms[11].alarmState;
      AlertData.alarm12 = ((uint32_t)alarms[12].internalState << 16) | alarms[12].alarmState;
      AlertData.alarm13 = ((uint32_t)alarms[13].internalState << 16) | alarms[13].alarmState;
      AlertData.alarm14 = 0;
      AlertData.alarm15 = 0;
      AlertData.alarm16 = 0;
      AlertData.alarm17 = 0;
      AlertData.alarm18 = 0;
      AlertData.alarm19 = 0;
      AlertData.alarm20 = 0;

      memcpy((void *)alert_payload, (void *)&AlertData, sizeof(struct AlertData));

      /*
       * Determine if an Alarm was detected
       */

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
   MQTTClient_unsubscribe(client, WELL_TOPIC);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
void MyMQTTPublish() {
   int rc;
   int i;
   time_t t;
   time(&t);

   char *alarm_names[20] = {"Low Water Alert",
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
                            "undefined",
                            "undefined"};

   if (verbose) {
      for (i = 0; i <= A_LEN; i++) {
         printf("%x ", alert_payload[i]);
      }
      printf("%s", ctime(&t));
   }

   pubmsg.payload = alert_payload;
   pubmsg.payloadlen = A_LEN * 4;
   pubmsg.qos = QOS;
   pubmsg.retained = 0;
   deliveredtoken = 0;

   if ((rc = MQTTClient_publishMessage(client, A_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS) {
      log_message("Alert: Error == Failed to Publish Message. Return Code: %d\n", rc);
      printf("Failed to publish message, return code %d\n", rc);
      rc = EXIT_FAILURE;
   }
   /* 
    * Publish JSON Message Only if an Alarm is active 
    */
   
   if (alarmSummary == 1) {
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
      MQTTClient_publishMessage(client, "JSON - Alert Data", &pubmsg, &token);
      MQTTClient_waitForCompletion(client, token, TIMEOUT);

      json_object_put(root); // Free the memory allocated to the JSON object
   }
}


void processAlarmWellPumpsNotStarting() {
   
   switch (alarms[7].internalState)
   {
   case (timeout):
      if (alarms[7].timeOut == 0)
      {
         alarms[7].internalState  = inactive;
      }
      else
      {
         alarms[7].timeOut--;
      }
      break;
   case (inactive):
      if (WellSensorData.House_tank_pressure_switch_on == ON)
      {
         alarms[7].internalState  = trigger;
         alarms[7].timer  = 0;
      }
      break;
   case (trigger):
      if (alarms[7].timer >= alarms[7].timeOut)
      {
         if ((WellSensorData.House_tank_pressure_switch_on == ON && (WellSensorData.well_pump_1_on== OFF || WellSensorData.well_pump_2_on == OFF)))
         {
            alarms[7].internalState  = active;
            alarms[7].timer  = 0;
         }
      }
      else
      {
         alarms[7].timer ++;
      }
      break;
   case (active):
      if ((WellSensorData.House_tank_pressure_switch_on == ON && (WellSensorData.well_pump_1_on == OFF || WellSensorData.well_pump_2_on == OFF)))
      {
         alarms[7].alarmState = 1;
      }
      else
      {
         alarms[7].internalState = reset;
      }
      break;
   case (reset):
      if (WellSensorData.House_tank_pressure_switch_on == OFF) {
         alarms[7].alarmState = 0;
         alarms[7].timer  = 0;
         alarms[7].timeOut = 10;
         alarms[7].internalState = timeout;
      }
      break;
   default:
      break;
   }
}
void processAlarmTankCriticallyLow(){
   switch (alarms[1].internalState)
   {
      case timeout:
         if (alarms[1].timeOut == 0)
         {
               alarms[1].internalState = inactive;
         }
         else
         {
               alarms[1].timeOut--;
         }
         break;

      case inactive:
         if (TankSensorData.float_state_4 == ON)
         {
               alarms[1].internalState = trigger;
               alarms[1].timer = 0;
         }
         break;

      case trigger:
         if (alarms[1].timer >= 60) // check if the sensor state has been ON for 60 seconds
         {
               alarms[1].internalState = active;
               alarms[1].timer = 0;
         }
         else
         {
               alarms[1].timer++;
         }
         break;

      case active:
         if (TankSensorData.float_state_4 == ON)
         {
               alarms[1].alarmState = 1; // alarm triggered
               log_message("Water Level in Tank is Critically Low");
         }
         else
         {
               alarms[1].internalState = reset;
         }
         break;

      case reset:
         if (TankSensorData.float_state_4 == OFF)
         {
               alarms[1].alarmState = 0; // reset alarm
               alarms[1].timer = 0;
               alarms[1].timeOut = 60; // reset timeout
               alarms[1].internalState = timeout;
         }
         break;

      default:
         break;
   }

}