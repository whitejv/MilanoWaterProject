#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <json-c/json.h>
#include <curl/curl.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "influxdb.h"
#include "../include/water.h"
//#include "../include/alert.h"
#include <stdbool.h> // Required for using 'bool' type

int verbose = FALSE;

#define INFLUXDB_HOST "http://192.168.1.88:8086"
#define INFLUXDB_TOKEN "RHl3fYEp8eMLtIUraVPzY4zp_hnnu2kYlR9hYrUaJLcq5mB2PvDsOi9SR0Tu_i-t_183fHb1a95BTJug-vAPVQ=="
#define INFLUXDB_ORG "Milano"
#define INFLUXDB_BUCKET "MWPWater"
#define INFLUXDB_PRECISION "s"

s_influxdb_client *influxdb_client;

void init_influxdb() {
    influxdb_client = influxdb_client_new(INFLUXDB_HOST, INFLUXDB_TOKEN, "", INFLUXDB_BUCKET, 0);
    if (influxdb_client == NULL) {
        fprintf(stderr, "Failed to create InfluxDB client\n");
        exit(1);
    }
}
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb;
}
void write_to_influxdb(int pump, int param1, int param2, float intervalFlow, float pressure, float amperage, float temperature) {
    CURL *curl;
    CURLcode res;
    char url[256];
    // Line protocol format: measurement,tag_key=tag_value field_key=field_value timestamp
    // In this case, no tags are added, and the fields are the metrics you're sending
    char data[512];

    snprintf(data, sizeof(data),
        "pump_metrics,pump_id=%d,param1=%d,param2=%d interval_flow=%.2f,pressure=%.2f,amperage=%.2f,temperature=%.2f",
        pump, param1, param2, intervalFlow, pressure, amperage, temperature);

    // InfluxDB v2 settings
    struct curl_slist *headers = NULL;
    char auth_header[256];

    curl = curl_easy_init();
    if(curl) {
        snprintf(url, sizeof(url), "%s/api/v2/write?org=%s&bucket=%s", INFLUXDB_HOST, INFLUXDB_ORG, INFLUXDB_BUCKET);
        snprintf(auth_header, sizeof(auth_header), "Authorization: Token %s", INFLUXDB_TOKEN);

        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: text/plain; charset=utf-8");
        printf("url: %s\n", url);
        printf("data: %s\n", data);
        printf("auth_header: %s\n", auth_header);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            printf("InfluxDB write status: %ld\n", response_code);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;

void MyMQTTPublish(void) ;


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
   int pump = 0;
   int param1 = 0;
   int param2 = 0;
   float intervalFlow = 0;
   float pressure = 0;
   float amperage = 0;
   float temperature = 0;

   log_message("Log: Started\n");

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
      log_message("Log: Error == Failed to Create Client. Return Code: %d\n", rc);
      printf("Failed to create client, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      log_message("Log: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
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
      log_message("Log: Error == Failed to Connect. Return Code: %d\n", rc);
      printf("Failed to connect, return code %d\n", rc);
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   printf("Subscribing to all monitor topics: %s\nfor client: %s using QoS: %d\n\n", "mwp/data/monitor/#", LOG_CLIENTID, QOS);
   log_message("Log: Subscribing to topic: %s for client: %s\n", "mwp/data/monitor/#", LOG_CLIENTID);
   MQTTClient_subscribe(client, "mwp/data/monitor/#", QOS);

   init_influxdb();

   /*
    * Main Loop
    */

   log_message("Log: Entering Main Loop\n");

   while (1)
   {
      if (wellMon_.well.well_pump_1_on == 1 ) {
         pump = 3;
        param1 = 1;
        param2 = 2;
        intervalFlow = houseMon_.house.intervalFlow  ;
        pressure = houseMon_.house.pressurePSI;
        amperage = wellMon_.well.amp_pump_1;
        temperature = houseMon_.house.temperatureF;
        // Write data to InfluxDB
        write_to_influxdb(pump, param1, param2, intervalFlow, pressure, amperage, temperature);
      }
      if (wellMon_.well.well_pump_3_on == 1 ) {
        pump = 3;
        param1 = 1;
        param2 = 2;
        intervalFlow = tankMon_.tank.intervalFlow;
        pressure = tankMon_.tank.pressurePSI;
        amperage = wellMon_.well.amp_pump_3;
        temperature = tankMon_.tank.temperatureF;
         //if (verbose) {
         printf("pump: %d, param1: %d, param2: %d, intervalFlow: %f, pressure: %f, amperage: %f, temperature: %f ", pump, param1, param2, intervalFlow, pressure, amperage, temperature);
         printf("%s", ctime(&t));
        //}
        // Write data to InfluxDB
        write_to_influxdb(pump, param1, param2, intervalFlow, pressure, amperage, temperature);
      }
      if (wellMon_.well.irrigation_pump_on == 1 ) {
        pump = 4;
        if ( irrigationMon_.irrigation.controller == 1 ) {
           param1 = 1;
           param2 = irrigationMon_.irrigation.zone;
        }
        else if ( irrigationMon_.irrigation.controller == 2 ) {
           param1 = 2;
           param2 = irrigationMon_.irrigation.zone;
        }
        else {
           param1 = 0;
           param2 = 0;
        }
        intervalFlow = irrigationMon_.irrigation.intervalFlow;
        pressure = irrigationMon_.irrigation.pressurePSI;
        amperage = wellMon_.well.amp_pump_4;
        temperature = irrigationMon_.irrigation.temperatureF;
         //if (verbose) {
         printf("pump: %d, param1: %d, param2: %d intervalFlow: %f, pressure: %f, amperage: %f, temperature: %f ", pump, param1, param2, intervalFlow, pressure, amperage, temperature);
         printf("%s", ctime(&t));
        //}
        // Write data to InfluxDB
      write_to_influxdb(pump, param1, param2, intervalFlow, pressure, amperage, temperature);
      }

      //MyMQTTPublish() ;

      /*
       * Run at this interval
       */

      sleep(1);
   }
   log_message("Log: Exiting Main Loop\n");
   MQTTClient_unsubscribe(client, "mwp/data/monitor/#");
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);

   // Don't forget to free the InfluxDB client when you're done
    influxdb_client_free(influxdb_client);

   return rc;
}
void MyMQTTPublish() {
   int rc;
   int i;
   time_t t;
   time(&t);

   pubmsg.payload = log_.data_payload;
   pubmsg.payloadlen = LOG_LEN * 4;
   pubmsg.qos = QOS;
   pubmsg.retained = 0;
   deliveredtoken = 0;

   if ((rc = MQTTClient_publishMessage(client, LOG_TOPICID, &pubmsg, &token)) != MQTTCLIENT_SUCCESS) {
      log_message("Log: Error == Failed to Publish Message. Return Code: %d\n", rc);
      printf("Failed to publish message, return code %d\n", rc);
      rc = EXIT_FAILURE;
   }

//  json_object *root = json_object_new_object();
/*
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
    //MQTTClient_publishMessage(client, ALERT_JSONID, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, TIMEOUT);

    json_object_put(root); // Free the memory allocated to the JSON object
*/      
}
