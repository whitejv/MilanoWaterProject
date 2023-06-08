
#define BLYNK_TEMPLATE_ID "TMPLrWkZjlCS"
#define BLYNK_DEVICE_NAME "Villa Milano Home"
#define BLYNK_AUTH_TOKEN "qGKZG9ptI-uLjGEG9y_8BSaTDunwLEQM"
// #define BLYNK_AUTH_TOKEN "50z7wkVf3LuA1G8_5QGjVn54lllSnpG3";

#define BLYNK_DEBUG
//#define BLYNK_PRINT stdout
#ifdef RASPBERRY
#include <BlynkApiWiringPi.h>
#else
#include <BlynkApiLinux.h>
#endif
#include <BlynkSocket.h>
#include <BlynkOptionsParser.h>

static BlynkTransportSocket _blynkTransport;
BlynkSocket Blynk(_blynkTransport);
static const char serv[] = "blynk.cloud";
static const char auth[] = BLYNK_AUTH_TOKEN;
static uint16_t port = 80;

#include <BlynkWidgets.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/water.h"
//#include "MQTTClient.h"
#include "MQTTAsync.h"

#define CLIENTID "Tank Blynker"

BlynkTimer tmr;
int verbose = FALSE;
int disc_finished = 0;
int subscribed = 0;
int finished = 0;

volatile MQTTAsync_token deliveredtoken;
MQTTAsync client;
MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
MQTTAsync_token token;
/* Using an include here to allow me to reuse a chunk of code that
   would not work as a library file. So treating it like an include to 
   copy and paste the same code into multiple programs. 
*/

#include "../mylib/msgarrvdAsync.c"

void connlost(void *context, char *cause)
{
        MQTTAsync client = (MQTTAsync)context;
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        int rc;
        printf("\nConnection lost\n");
        printf("     cause: %s\n", cause);
        printf("Reconnecting\n");
        conn_opts.keepAliveInterval = 120;
        conn_opts.cleansession = 1;
        if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start connect, return code %d\n", rc);
            finished = 1;
        }
}
void onDisconnect(void* context, MQTTAsync_successData* response)
{
        printf("Successful disconnection\n");
        disc_finished = 1;
}
void onSubscribe(void* context, MQTTAsync_successData* response)
{
        printf("Subscribe succeeded\n");
        subscribed = 1;
}
void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
        printf("Subscribe failed, rc %d\n", response ? response->code : 0);
        finished = 1;
}
void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
        printf("Connect failed, rc %d\n", response ? response->code : 0);
        finished = 1;
}
void onConnect(void* context, MQTTAsync_successData* response)
{
        MQTTAsync client = (MQTTAsync)context;
        MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        int rc;
        printf("Successful connection\n");
        opts.onSuccess = onSubscribe;
        opts.onFailure = onSubscribeFailure;
        opts.context = client;
        deliveredtoken = 0;
        
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", M_TOPIC, CLIENTID, QOS);
        if ((rc = MQTTAsync_subscribe(client, M_TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe %s Topic, return code %d\n", M_TOPIC, rc);
                exit(EXIT_FAILURE);
        }
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TANK_TOPIC, CLIENTID, QOS);
        if ((rc = MQTTAsync_subscribe(client, TANK_TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe %s Topic, return code %d\n", TANK_TOPIC, rc);
                exit(EXIT_FAILURE);
        }
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
        "Press Q<Enter> to quit\n\n", FLOW_TOPIC, CLIENTID, QOS);
        if ((rc = MQTTAsync_subscribe(client, FLOW_TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe %s Topic, return code %d\n", FLOW_TOPIC, rc);
                exit(EXIT_FAILURE);
        }
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
        "Press Q<Enter> to quit\n\n", WELL_TOPIC, CLIENTID, QOS);
        if ((rc = MQTTAsync_subscribe(client, WELL_TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe %s Topic, return code %d\n", WELL_TOPIC, rc);
                exit(EXIT_FAILURE);
        }
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", A_TOPIC, CLIENTID, QOS);
        if ((rc = MQTTAsync_subscribe(client, A_TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe %s Topic, return code %d\n", A_TOPIC, rc);
                exit(EXIT_FAILURE);
        }
}
void loop()
{
   static int OneTime = 0;
   u_int32_t BitPackedPayload = 0;
   int PumpRunCount[5];
   int floatState[5];
   int floatLedcolor[5];

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
   /*
    char *pumpMenu[] = { "OFF",
    "ON",
    "AUTO",
    "ERROR" };
    */

   /*
    * Jazz up the basic collors
    */
   // Blynk.setProperty(3, "color", "0x01ffB0");

   /*
    * Unpack the Data for Some Items
    */
   printf("~");
   BitPackedPayload = monitor_payload[8];
   PumpRunCount[4] = (BitPackedPayload & 0xff000000) >> 24;
   PumpRunCount[3] = (BitPackedPayload & 0x00ff0000) >> 16;
   PumpRunCount[2] = (BitPackedPayload & 0x0000ff00) >> 8;
   PumpRunCount[1] = (BitPackedPayload & 0x000000ff);

   BitPackedPayload = monitor_payload[13];
   floatState[4] = (BitPackedPayload & 0xffff0000) >> 16;
   floatState[3] = (BitPackedPayload & 0x0000ffff);
   BitPackedPayload = monitor_payload[14];
   floatState[2] = (BitPackedPayload & 0xffff0000) >> 16;
   floatState[1] = (BitPackedPayload & 0x0000ffff);

   BitPackedPayload = monitor_payload[15];
   floatLedcolor[4] = (BitPackedPayload & 0xff000000) >> 24;
   floatLedcolor[3] = (BitPackedPayload & 0x00ff0000) >> 16;
   floatLedcolor[2] = (BitPackedPayload & 0x0000ff00) >> 8;
   floatLedcolor[1] = (BitPackedPayload & 0x000000ff);

   Blynk.setProperty(5, "color", ledcolorPalette[monitor_payload[4]]);  // Set LED Label to HEX colour
   Blynk.setProperty(6, "color", ledcolorPalette[monitor_payload[5]]);  // Set LED Label to HEX colour
   Blynk.setProperty(7, "color", ledcolorPalette[monitor_payload[6]]);  // Set LED Label to HEX colour
   Blynk.setProperty(20, "color", ledcolorPalette[monitor_payload[7]]); // Set LED Label to HEX colour

   Blynk.setProperty(12, "color", ledcolorPalette[floatLedcolor[1]]); // Set LED Label to HEX colour
   Blynk.setProperty(13, "color", ledcolorPalette[floatLedcolor[2]]); // Set LED Label to HEX colour
   Blynk.setProperty(14, "color", ledcolorPalette[floatLedcolor[3]]); // Set LED Label to HEX colour
   Blynk.setProperty(15, "color", ledcolorPalette[floatLedcolor[4]]); // Set LED Label to HEX colour

   Blynk.setProperty(4, "color", ledcolorPalette[monitor_payload[17]]);
   Blynk.setProperty(21, "color", ledcolorPalette[monitor_payload[19]]); // Set LED Label to HEX colour

   /***  SEND INFO TO BLYNK     ***/

   // Blynk.virtualWrite (V	0	, blynk_payload[0])	;   //Unused
   Blynk.virtualWrite(V1, tank_sensor_payload[1]);         // Level ft
   Blynk.virtualWrite(V2, tank_sensor_payload[2]);         // Gallons
   Blynk.virtualWrite(V3, tank_sensor_payload[3]);         // Level %
   Blynk.virtualWrite(V4, monitor_payload[16]);          // Septic Alert
   Blynk.virtualWrite(V5, monitor_payload[0]);           // Pump Current Sense P1
   Blynk.virtualWrite(V6, monitor_payload[1]);           // Pump Current Sense P2
   Blynk.virtualWrite(V7, monitor_payload[2]);           // Pump Current Sense P3
   Blynk.virtualWrite(V8, flow_sensor_payload[10]);             // irrigation pump temperature
   Blynk.virtualWrite(V9, (int)well_sensor_payload[12]);    // Faults
   Blynk.virtualWrite(V10, (int)tank_sensor_payload[10]);  // Cycle Count
   Blynk.virtualWrite(V11, tank_sensor_payload[11]);       // System Temperature f
   Blynk.virtualWrite(V12, floatState[4]);                      // Float 1 Hi
   Blynk.virtualWrite(V13, floatState[3]);                      // Float 2 90%
   Blynk.virtualWrite(V14, floatState[2]);                      // Float 3 50%
   Blynk.virtualWrite(V15, floatState[1]);                      // Float 4 Low
   Blynk.virtualWrite(V16, flow_sensor_payload[0]);             // Gallons per Minute (Rolling Average)
   Blynk.virtualWrite(V17, flow_sensor_payload[1]);             // Daily Total Gallons
   Blynk.virtualWrite(V18, flow_sensor_payload[2]);             // irrigation System Pressure PSI
   //Blynk.virtualWrite(V19, formatted_sensor_payload[17]);       // House Water Press
   Blynk.virtualWrite(V20, monitor_payload[3]);          // Pump Current Sense P4
   Blynk.virtualWrite(V21, monitor_payload[18]);         // Home Tank Pressure Relay Sense
   Blynk.virtualWrite(V22, PumpRunCount[1]);                    // Pump Run Count P1
   Blynk.virtualWrite(V23, PumpRunCount[2]);                    // Pump Run Count P2
   Blynk.virtualWrite(V24, PumpRunCount[3]);                    // Pump Run Count P3
   Blynk.virtualWrite(V25, PumpRunCount[4]);                    // Pump Run Count P4
   Blynk.virtualWrite(V26, (monitor_payload[9] / 60.));  // PumpRunTime P1
   Blynk.virtualWrite(V27, (monitor_payload[10] / 60.)); // PumpRunTime P2
   Blynk.virtualWrite(V28, (monitor_payload[11] / 60.)); // PumpRunTime P3
   Blynk.virtualWrite(V29, (monitor_payload[12] / 60.)); // PumpRunTime P4
   Blynk.virtualWrite(V30, (flow_sensor_payload[3]));           // irrigation pump temperature
   Blynk.virtualWrite(V31, (tank_sensor_payload[0])); // PumpRunTime P4
   Blynk.virtualWrite(V32, (tank_sensor_payload[4]));           // irrigation pump temperature
   /*

    Blynk.virtualWrite(V0, blynk_payload[0]);  //Press Sensor Val
    Blynk.virtualWrite(V1, formatted_sensor_payload[1]);  //Level ft
    Blynk.virtualWrite(V2, formatted_sensor_payload[2]);  //Gallons
    Blynk.virtualWrite(V3, formatted_sensor_payload[3]);  //Level %
    //Blynk.virtualWrite(V4, monitor_sensor_payload[4]);  //Total GPM
    Blynk.virtualWrite(V5, monitor_sensor_payload[0]); //LED 1 Well 1
    Blynk.virtualWrite(V6, monitor_sensor_payload[1]); //LED 2 Well 2
    Blynk.virtualWrite(V7, monitor_sensor_payload[2]); //LED 3 Well 3
    //Blynk.virtualWrite(V8, (int)formatted_sensor_payload[8]);   //FW Ver
    Blynk.virtualWrite(V9, (int)formatted_sensor_payload[9]);   //Faults
    Blynk.virtualWrite(V10,(int)formatted_sensor_payload[10]);  //Cycle Count
    Blynk.virtualWrite(V11, formatted_sensor_payload[11]); //Temperature f
    Blynk.virtualWrite(V12, floatState[4]); //Float 1 Hi
    Blynk.virtualWrite(V13, floatState[3]); //Float 2 90%
    Blynk.virtualWrite(V14, floatState[2]); //Float 3 50%
    Blynk.virtualWrite(V15, floatState[1]); //Float 4 Low
    //Blynk.virtualWrite(V16, (int)blynk_payload[23]);//Run Commanded P1
    //Blynk.virtualWrite(V17, (int)blynk_payload[24]);//Run Commanded P2
    //Blynk.virtualWrite(V18, (int)blynk_payload[25]);//Run Commanded P3
    Blynk.virtualWrite(V19, formatted_sensor_payload[17]);//House Water Press
    Blynk.virtualWrite(V20, monitor_sensor_payload[3]);//LED 4 Spinkler Pump
    Blynk.virtualWrite(V21, monitor_sensor_payload[18]);//LED 5 Home Pres SW
    Blynk.virtualWrite(V22, PumpRunCount[1]); //Pump Run Count
    Blynk.virtualWrite(V23, PumpRunCount[2]); //Pump Run Count
    Blynk.virtualWrite(V24, PumpRunCount[3]); //Pump Run Count
    Blynk.virtualWrite(V25, PumpRunCount[4]); //Pump Run Count
    Blynk.virtualWrite(V26, (monitor_sensor_payload[9]/60.));//PumpRunTime
    Blynk.virtualWrite(V27, (monitor_sensor_payload[10]/60.));//PumpRunTime
    Blynk.virtualWrite(V28, (monitor_sensor_payload[11]/60.));//PumpRunTime
    Blynk.virtualWrite(V29, (monitor_sensor_payload[12]/60.));//PumpRunTime
    Blynk.virtualWrite(V30, (flow_sensor_payload[3]));//irrigation pump temperature
    Blynk.virtualWrite(V16, (flow_sensor_payload[0]));//irrigation pump temperature
    Blynk.virtualWrite(V17, (flow_sensor_payload[1]));//irrigation pump temperature
    Blynk.virtualWrite(V18, (flow_sensor_payload[2]));//irrigation pump temperature
    Blynk.virtualWrite(V8, (flow_sensor_payload[10]));//irrigation pump temperature
    Blynk.virtualWrite(V4, (monitor_sensor_payload[16]));//Septic Alert

    */

   if (alert_payload[0] == 1 && OneTime == 0)
   {
      OneTime = 1;
      Blynk.logEvent("pump_no_start");
      printf("pump_no_start\n");
   }

}

int main(int argc, char *argv[])
{
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
   
   int rc;
   int ch;
   MQTTAsync_create(&client, mqtt_address, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
   MQTTAsync_setCallbacks(client, NULL, connlost, msgarrvdAsync, NULL);
   conn_opts.keepAliveInterval = 120;
   conn_opts.cleansession = 1;
   conn_opts.onSuccess = onConnect;
   conn_opts.onFailure = onConnectFailure;
   conn_opts.context = client;
   if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
   {
            printf("Failed to start connect, return code %d\n", rc);
            exit(EXIT_FAILURE);
   }

   //log_message("Blynk: Started\n");

   printf("Connecting to Blynk: %s, %s, %d\n", serv, auth, port);

   // parse_options(argc, argv, auth, serv, port);
   Blynk.begin(auth, serv, port);
   
   tmr.setInterval(1000, loop);
   

   while (true)
   {

      //loop();
      Blynk.run() ;
      tmr.run();
   }

   //log_message("Blynk: Exited Main Loop\n");
   disc_opts.onSuccess = onDisconnect;
   if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
   {
            printf("Failed to start disconnect, return code %d\n", rc);
            exit(EXIT_FAILURE);
   }

exit:
   MQTTAsync_destroy(&client);
   return rc;
}
