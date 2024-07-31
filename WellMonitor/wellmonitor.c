#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <json-c/json.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

int verbose = FALSE;

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

   int i;
   time_t t;
   time(&t);
 
   int SepticAlert = 0;

   int raw_voltage1_adc = 0;
   int raw_voltage2_adc = 0;
   int raw_voltage3_adc = 0;
   int raw_voltage4_adc = 0;
   int pump1_on = 0;
   int pump2_on = 0;
   int pump3_on = 0;
   int pump4_on = 0;

   float WaterPresSensorValue;

   int PressSwitState;

   int raw_temp = 0;
   float AmbientTempF = 0;
   float AmbientTempC = 0;

   

   log_message("WellMonitor: Started\n");

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

   if ((rc = MQTTClient_create(&client, mqtt_address, WELLMON_CLIENTID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      log_message("WellMonitor: Error == Failed to Create Client. Return Code: %d\n", rc);
      printf("Failed to create client, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      log_message("WellMonitor: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
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
      log_message("WellMonitor: Error == Failed to Connect. Return Code: %d\n", rc);
      printf("Failed to connect, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", WELLSENS_TOPICID, WELLSENS_CLIENTID, QOS);
   log_message("WellMonitor: Subscribing to topic: %s for client: %s\n", WELLSENS_TOPICID, WELLSENS_CLIENTID);
   MQTTClient_subscribe(client, WELLSENS_TOPICID, QOS);

   /*
    * Main Loop
    */

   log_message("WellMonitor: Entering Main Loop\n");

   while (1)
   {
      time(&t);

     
      raw_voltage1_adc = wellSens_.well.adc_x1;
      if (raw_voltage1_adc > 1000) {
         pump1_on = 1 ;
      }
      else {
         pump1_on = 0;
      }

      raw_voltage2_adc = wellSens_.well.adc_x2;
      if (raw_voltage2_adc > 1000) {
         pump2_on = 1 ;
      }
      else {
         pump2_on = 0;
      }     
      raw_voltage3_adc = wellSens_.well.adc_x3;
      if (raw_voltage3_adc > 1000) {
         pump3_on = 1 ;
      }
      else {
         pump3_on = 0;
      }
      raw_voltage4_adc = wellSens_.well.adc_x4;
      if (raw_voltage4_adc > 750) {
         pump4_on = 1 ;
      }
      else {
         pump4_on = 0;
      }

      /*
       * Convert the Discrete data
       */


      SepticAlert = !((wellSens_.well.GPIO_x1 & 0x02)>1);
      PressSwitState = !(wellSens_.well.GPIO_x1 & 0x01);

      /*
       * Convert Raw Temp Sensor to degrees farenhiet
       */

      raw_temp = wellSens_.well.temp;
      //AmbientTempC = raw_temp ;
      AmbientTempF = raw_temp;
      //printf("Ambient Temp:%f  \n", AmbientTempF);

      /*
       * Set Firmware Version
       * firmware = well_data_payload[20] & SubFirmware;
       */

      /*
       * Load Up the Data
       */

      wellMon_.well.well_pump_1_on = pump1_on;
      wellMon_.well.well_pump_2_on = pump2_on;
      wellMon_.well.well_pump_3_on = pump3_on;
      wellMon_.well.irrigation_pump_on = pump4_on;
      wellMon_.well.house_water_pressure = 0.0;
      wellMon_.well.system_temp = AmbientTempF;
      wellMon_.well.House_tank_pressure_switch_on = PressSwitState;
      wellMon_.well.septic_alert_on = SepticAlert;
      wellMon_.well.cycle_count = wellSens_.well.cycle_count;
      wellMon_.well.fw_version = wellSens_.well.fw_version;
      wellMon_.well.amp_pump_1 = wellSens_.well.adc_x1;
      wellMon_.well.amp_pump_2 = wellSens_.well.adc_x2;
      wellMon_.well.amp_pump_3 = wellSens_.well.adc_x3;
      wellMon_.well.amp_pump_4 = wellSens_.well.adc_x4;
      wellMon_.well.amp_5 = wellSens_.well.adc_x5;
      wellMon_.well.amp_6 = wellSens_.well.adc_x6;
      wellMon_.well.amp_7 = wellSens_.well.adc_x7;
      wellMon_.well.amp_8 = wellSens_.well.adc_x8;

      /*
       * Load Up the Payload
       */
       if (verbose) {
         for (i=0; i<=WELLMON_LEN-1; i++) {
            printf("%.3f ", wellMon_.data_payload[i]);
         }
         printf("%s", ctime(&t));
      }
      pubmsg.payload = wellMon_.data_payload;
      pubmsg.payloadlen = WELLMON_LEN * 4;
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      deliveredtoken = 0;
      if ((rc = MQTTClient_publishMessage(client, WELLMON_TOPICID, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
      {
         log_message("WellMonitor: Error == Failed to Publish Message. Return Code: %d\n", rc);
         printf("Failed to publish message, return code %d\n", rc);
         rc = EXIT_FAILURE;
      }
      json_object *root = json_object_new_object();
      for (i=0; i<=WELLMON_LEN-1; i++) {
         json_object_object_add(root, wellmon_ClientData_var_name [i], json_object_new_double(wellMon_.data_payload[i]));
      }

      const char *json_string = json_object_to_json_string(root);

      pubmsg.payload = (void *)json_string; // Make sure to cast the const pointer to void pointer
      pubmsg.payloadlen = strlen(json_string);
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      MQTTClient_publishMessage(client, WELLMON_JSONID, &pubmsg, &token);
      //printf("Waiting for publication of %s\non topic %s for client with ClientID: %s\n", json_string, TANK_TOPIC, TANK_MONID);
      MQTTClient_waitForCompletion(client, token, TIMEOUT);
      //printf("Message with delivery token %d delivered\n", token);

      json_object_put(root); // Free the memory allocated to the JSON object
      /*
       * Run at this interval
       */

      sleep(1);
   }
   log_message("WellMonitor: Exiting Main Loop\n") ;
   MQTTClient_unsubscribe(client, WELLMON_TOPICID);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
