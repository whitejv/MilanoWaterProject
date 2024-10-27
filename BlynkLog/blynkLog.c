#define ADDRESS     "blynk.cloud:1883"   // Blynk MQTT Server
#define CLIENTID    "Log Blynk"               // Unique client ID for your device
#define BLYNK_TOPIC       "batch_ds"
#define BLYNK_DS "ds"
#define BLYNK_PROP_COLOR "prop/color"
#define BLYNK_PROP_LABEL "prop/label"
#define BLYNK_EVENT "event"


#define BLYNK_TEMPLATE_ID "TMPLuui_eIOW"
#define BLYNK_TEMPLATE_NAME "VillaMilano Alerts"
#define BLYNK_DEVICE_NAME "device"
#define BLYNK_AUTH_TOKEN "vrCmamlLChWJ2dgueIoQXLWqnur2puAO"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <json-c/json.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"
#include "../include/alert.h"
#include <stdbool.h> // Required for using 'bool' type

int verbose = FALSE;
int disc_finished = 0;
int subscribed = 0;
int finished = 0;
int clearCommand = 0;
int muteCommand = 0;
MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
   //printf("Message with token value %d delivery confirmed\n", dt);
   deliveredtoken = dt;
}

/* Using an include here to allow me to reuse a chunk of code that
   would not work as a library file. So treating it like an include to 
   copy and paste the same code into multiple programs. 
*/
#include "../mylib/msgarrvd.c"
int blynkarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
   int i;
   
   if (verbose) {
      printf("Message arrived:\n");
      printf("          topic: %s  ", topicName);
      printf("         length: %d  ", topicLen);
      printf("     PayloadLen: %d\n", message->payloadlen);
      printf("message: %s\n", message->payload);
   }

   if ( strcmp(topicName, "downlink/ds/Clear") == 0) {
      if (strcmp(message->payload, "1") == 0) {;
         clearCommand = 1;
      }
      else {
         clearCommand = 0;
      }
      printf("-->Command Recv'd: Clear Command: %s\n", message->payload);
   }
   if ( strcmp(topicName, "downlink/ds/Mute") == 0) {
      if (strcmp(message->payload, "1") == 0) {;
         muteCommand = 1;
      }
      else {
         muteCommand = 0;
      }
      printf("-->Command Recv'd: Mute Command: %s\n", message->payload);
   }

   MQTTClient_freeMessage(&message);
   MQTTClient_free(topicName);
   return 1;
}  
void connlost(void *context, char *cause)
{
   printf("\nConnection lost\n");
   printf("     cause: %s\n", cause);
}

int loop(MQTTClient blynkClient)
{
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   int rc;
   int i;
   int alarmValue = 0;
   char topic[256];   // Buffer for topic
   char payload[256]; // Buffer for payload
   static int initProperties = 0;
   
   typedef struct {
    unsigned int alarmState   : 8;  // 8-bit field for alarm state
    unsigned int eventSend    : 8;  // 8-bit field for event send flag
    unsigned int occurences   : 8;  // 8-bit field for occurrences
    unsigned int internalState: 8;  // 8-bit field for internal state
   } alarmStatus;

   alarmStatus alarmStat[ALARM_COUNT]; // Array of alarmStatus structures
   

   const char *ledcolor[] = {"Green",
                             "Blue",
                             "Orange",
                             "Red",
                             "Yellow",
                             "Purple",
                             "Fuscia",
                             "Black"};

   const char *ledcolorPalette[] = {"ff0000",  // red
                                    "ffff00",  // yellow
                                    "00ff00",  // green
                                    "0000FF",  // blue
                                    "ff8000",  // orange
                                    "bf00ff",  // purple
                                    "fe2e9a",  // fuscia
                                    "000000"}; // black
  

   if (initProperties == 0) {
      for (i=0; i<=5; i++) {
         // Set label
         sprintf(topic, "%s/%s/%s", BLYNK_DS, alert_ClientData_var_name[i], BLYNK_PROP_LABEL);
         strcpy(payload, alarmInfo[i].label);
         pubmsg.payload = payload;
         //printf("Label Payload: %s %s\n", topic, payload);
         pubmsg.payloadlen = strlen(payload);
         pubmsg.qos = 1;
         pubmsg.retained = 0;
         MQTTClient_publishMessage(blynkClient, topic, &pubmsg, &token);

         // Set color
         sprintf(topic, "%s/%s/%s", BLYNK_DS, alert_ClientData_var_name[i], BLYNK_PROP_COLOR);
         strcpy(payload, ledcolorPalette[alarmInfo[i].type]); // Set LED color to HEX colour from palette
         pubmsg.payload = payload;
         //printf("Color Payload: %s %s\n", topic, payload);
         pubmsg.payloadlen = strlen(payload);
         MQTTClient_publishMessage(blynkClient, topic, &pubmsg, &token);
      }
      initProperties = 60;  // refresh properties every 60 seconds
   }
   else {
      initProperties--;
   }     
   // Memcpy the payload to my bit packed structure
   
   memcpy(alarmStat, alert_.data_payload, sizeof(alert_.data_payload));
   for (i=0; i<=5; i++) {
      //printf("%x\n", alert_.data_payload[i]);
      //printf("Alarm %d: State: %d, Event: %d, Occurrences: %d, Internal: %d\n", i, alarmStat[i].alarmState, alarmStat[i].eventSend, alarmStat[i].occurences, alarmStat[i].internalState);
   }
   /* 
    * Send the alarm data to Blynk 100=active alarm; 0=inactive alarm; 1-99=occurrences
    */

   json_object *root = json_object_new_object();
   
   for (i=0; i<=5; i++) {
      if (alarmStat[i].alarmState == active) {
         alarmValue = 100;  // full scale if alarm is active
      }
      else {
         alarmValue = alarmStat[i].occurences;    // Set to show occurrences since last reset
      }
      json_object_object_add(root, alert_ClientData_var_name [i], json_object_new_int((alarmValue)));
   }

   const char *json_string = json_object_to_json_string(root);
   pubmsg.payload = (void *)json_string; // Make sure to cast the const pointer to void pointer
   pubmsg.payloadlen = strlen(json_string);
   pubmsg.qos = QOS;
   pubmsg.retained = 0;
   //printf("Topic: %s Payload: %s\n", BLYNK_TOPIC, json_string);
   // Publish the message to the specified topic
   if ((rc = MQTTClient_publishMessage(blynkClient, BLYNK_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS) {
      printf("Failed to publish message, return code %d\n", rc);
      MQTTClient_disconnect(blynkClient, 10000);
      MQTTClient_destroy(&blynkClient);
      return EXIT_FAILURE;
   }

   // Wait for the message to be delivered
   //printf("Waiting for up to %ld seconds for publication of message on topic %s...\n", TIMEOUT/1000, BLYNK_TOPIC);
   rc = MQTTClient_waitForCompletion(blynkClient, token, TIMEOUT);
   //printf("Message with delivery token %d delivered\n", token);

   /* 
    * Send the alarm event to Blynk - Only once per alarm event
    */

   
   if (muteCommand == 0) {
      for (i=0; i<=5; i++) {
         if (alarmStat[i].eventSend == 1) {
            // Set Event Message
            sprintf(topic, "%s/%s", BLYNK_EVENT, alert_ClientData_var_name[i]);
            strcpy(payload, alarmInfo[i].eventMessage);
            pubmsg.payload = payload;
            printf("Event Payload: %s %s\n", topic, payload);
            pubmsg.payloadlen = strlen(payload);
            pubmsg.qos = 1;
            pubmsg.retained = 0;
            MQTTClient_publishMessage(blynkClient, topic, &pubmsg, &token);
         }
      }
   }
   return 0;
}

int main(int argc, char *argv[])
{
   
   MQTTClient client;
   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   int rc;
   int i;
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
   
   //log_message("Blynk: Started\n");

   if ((rc = MQTTClient_create(&client, mqtt_address, CLIENTID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to create client, return code %d\n", rc);
      //log_message("Blynk: Error == Failed to Create Client. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to set callbacks, return code %d\n", rc);
      //log_message("Blynk: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   conn_opts.keepAliveInterval = 120;
   conn_opts.cleansession = 1;
   //conn_opts.username = mqttUser;       //only if req'd by MQTT Server
   //conn_opts.password = mqttPassword;   //only if req'd by MQTT Server
   if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
   {
      printf("Failed to connect, return code %d\n", rc);
      //log_message("Blynk: Error == Failed to Connect. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }


//Connecting to Blynk MQTT Server at tcp://mqtt.blynk.cc:1883
   //printf("Connecting to Blynk: %s, %s, %d\n", serv, auth, port);
    MQTTClient blynkClient;
    MQTTClient_connectOptions blynk_conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message blynk_pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken blynk_token;
    //int rc;

    // Create MQTT client
    if ((rc = MQTTClient_create(&blynkClient, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to create client, return code %d\n", rc);
        return EXIT_FAILURE;
    }

    // Set MQTT connection options
    blynk_conn_opts.username = BLYNK_DEVICE_NAME;
    blynk_conn_opts.password = BLYNK_AUTH_TOKEN;
    blynk_conn_opts.keepAliveInterval = 45;
    blynk_conn_opts.cleansession = 1;
   
    
    // Connect to the Blynk MQTT server
    printf("Connecting to Blynk MQTT server at %s\n", ADDRESS);
    if ((rc = MQTTClient_connect(blynkClient, &blynk_conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        MQTTClient_destroy(&blynkClient);
        return EXIT_FAILURE;
    }
    if ((rc = MQTTClient_setCallbacks(blynkClient, NULL, connlost, blynkarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
      printf("Failed to set callbacks, return code %d\n", rc);
      //log_message("Blynk: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
    }
    printf("Connected to Blynk MQTT server at %s\n", ADDRESS);
    MQTTClient_subscribe(blynkClient, "downlink/ds/#", QOS);   
    printf("Subscribing to topics: downlink/ds/# for client: %s using QoS: %d\n\n", ALERT_TOPICID, ALERT_CLIENTID, QOS);


   printf("Subscribing to topics: %s\nfor client: %s using QoS: %d\n\n", ALERT_TOPICID, ALERT_CLIENTID, QOS);
   log_message("BlynkW: Subscribing to topic: %s for client: %s\n", ALERT_TOPICID, ALERT_CLIENTID);
   MQTTClient_subscribe(client, ALERT_TOPICID, QOS);

   while (TRUE)
   {

      loop(blynkClient);
      sleep(1);

   }

   //log_message("Blynk: Exited Main Loop\n");
   // Disconnect from the MQTT server
   MQTTClient_disconnect(blynkClient, 10000);
   MQTTClient_destroy(&blynkClient);

   //log_message("Blynk: Exited Main Loop\n");
   MQTTClient_unsubscribe(client, ALERT_CLIENTID);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}