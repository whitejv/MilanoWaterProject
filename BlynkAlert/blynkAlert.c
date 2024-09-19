#define ADDRESS     "blynk.cloud:1883"   // Blynk MQTT Server
#define CLIENTID    "Alert Blynk"               // Unique client ID for your device
#define BLYNK_TOPIC       "batch_ds" // Topic for Virtual Pin V1

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

int verbose = FALSE;
int disc_finished = 0;
int subscribed = 0;
int finished = 0;

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

void connlost(void *context, char *cause)
{
   printf("\nConnection lost\n");
   printf("     cause: %s\n", cause);
}

void loop(MQTTClient blynkClient)
{
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   int rc;

   const char *ledcolor[] = {"Green",
                             "Blue",
                             "Orange",
                             "Red",
                             "Yellow",
                             "Purple",
                             "Fuscia",
                             "Black"};

   const char *ledcolorPalette[] = {"0x00ff00",  // green
                                    "0x0000FF",  // blue
                                    "0xff8000",  // orange
                                    "0xff0000",  // red
                                    "0xffff00",  // yellow
                                    "0xbf00ff",  // purple
                                    "0xfe2e9a",  // fuscia
                                    "0x000000"}; // black
  

   //Blynk.setProperty(1, "color", ledcolorPalette[0]);  // Set LED Label to HEX colour
      strcpy(payload,   ledcolorPalette[floatLedcolor[1]]); // Set LED Label to HEX colour
   MQTTClient_publishMessage(blynkClient, "ds/Float 100/prop/color", &pubmsg, &token);
   //Blynk.setProperty(13, "color", ledcolorPalette[floatLedcolor[2]]); // Set LED Label to HEX colour
   strcpy(payload,  ledcolorPalette[floatLedcolor[2]]); // Set LED Label to HEX colour
   MQTTClient_publishMessage(blynkClient, "ds/Float 50/prop/color", &pubmsg, &token);
   //Blynk.setProperty(15, "color", ledcolorPalette[floatLedcolor[4]]); // Set LED Label to HEX colour

   strcpy(payload,  ledcolorPalette[monitor_.monitor.press_led_color]);
   MQTTClient_publishMessage(blynkClient, "ds/Pressure SW/prop/color", &pubmsg, &token);
   strcpy(payload,  ledcolorPalette[monitor_.monitor.septic_relay_alert_color]); // Set LED Label to HEX colour
   MQTTClient_publishMessage(blynkClient, "ds/Septic Alert/prop/color", &pubmsg, &token);
   /***  SEND INFO TO BLYNK     ***/

   const char* payload = "55";
   pubmsg.payload = (void*)payload;
   pubmsg.payloadlen = strlen(payload);
   pubmsg.qos = QOS;
   pubmsg.retained = 0;

   // Publish the message to the specified topic
   if ((rc = MQTTClient_publishMessage(blynkClient, BLYNK_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS) {
      printf("Failed to publish message, return code %d\n", rc);
      MQTTClient_disconnect(blynkClient, 10000);
      MQTTClient_destroy(&blynkClient);
      //return EXIT_FAILURE;
   }

   // Wait for the message to be delivered
   printf("Waiting for up to %ld seconds for publication of message on topic %s...\n", TIMEOUT/1000, BLYNK_TOPIC);
   rc = MQTTClient_waitForCompletion(blynkClient, token, TIMEOUT);
   printf("Message with delivery token %d delivered\n", token);

}

int main(int argc, char *argv[])
{   
   MQTTClient client;
   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
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

   printf("Subscribing to topic: %s using QoS: %d\n\n", ALERT_TOPICID, QOS);
   //log_message("Blynk: Subscribing to topic: %s for client: %s\n", ALERT_TOPICID, ALERT_CLIENTID);
   MQTTClient_subscribe(client, ALERT_TOPICID, QOS);

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
    printf("Connected to Blynk MQTT server at %s\n", ADDRESS);
   
   //tmr.setInterval(1000, loop);
   

   while (TRUE)
   {

      loop(blynkClient);
      sleep(1);
      //Blynk.run() ;
      //tmr.run();
   }

   //log_message("Blynk: Exited Main Loop\n");
   // Disconnect from the MQTT server
   MQTTClient_disconnect(blynkClient, 10000);
   MQTTClient_destroy(&blynkClient);

   MQTTClient_unsubscribe(client, ALERT_TOPICID);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
