
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
static const char serv[] = "blynk-cloud.com";
static const char auth[] = "50z7wkVf3LuA1G8_5QGjVn54lllSnpG3";
static uint16_t port = 8442;

#include <BlynkWidgets.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/water.h"
#include "MQTTAsync.h"

#define CLIENTID        "Tank Blynker"

float blynk_payload [30];
unsigned int recv_payload[100];


BlynkTimer tmr;

volatile MQTTAsync_token deliveredtoken;
int disc_finished = 0;
int subscribed = 0;
int finished = 0;
void connlost(void *context, char *cause)
{
        MQTTAsync client = (MQTTAsync)context;
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        int rc;
        printf("\nConnection lost\n");
        printf("     cause: %s\n", cause);
        printf("Reconnecting\n");
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start connect, return code %d\n", rc);
            finished = 1;
        }
}
int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    int i;
    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s  ", topicName);
    printf("     length: %d  ", topicLen);
    printf("     PayloadLen: %d  ", message->payloadlen);
    printf("   message: ");
    payloadptr = (char *)message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        printf("%x ", *payloadptr++);
    }
    putchar('\n'); 

    if (strcmp(topicName, F_TOPIC) == 0) {
       for(i=0; i<message->payloadlen/4; i++)
       {
          formatted_sensor_payload[i] = (float)*payloadptr++ ;
          printf("%3f ", formatted_sensor_payload[i]);
       }

       printf("\n");
    }
    else if (strcmp(topicName, M_TOPIC) == 0) {
    }
    else if (strcmp(topicName, A_TOPIC) == 0){
    }
    else {
         printf("Unknown Topic Recieved: %s\n", topicName ) ;
    }

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
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
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", F_TOPIC, CLIENTID, QOS);
        opts.onSuccess = onSubscribe;
        opts.onFailure = onSubscribeFailure;
        opts.context = client;
        deliveredtoken = 0;
        if ((rc = MQTTAsync_subscribe(client, F_TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe Formatted Topic, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", M_TOPIC, CLIENTID, QOS);
        if ((rc = MQTTAsync_subscribe(client, M_TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe Monitor Topic, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", A_TOPIC, CLIENTID, QOS);
        if ((rc = MQTTAsync_subscribe(client, A_TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe Alert Topic, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
}
void loop()
{
    
    char *ledcolor[] = { "Green", "Blue", "Orange", "Red" } ; // to help with debug print
    
    char *ledcolorPalette[] = { "0x00ff00",   //green
                                "0x0000FF",   //blue
                                "0xffff00",   //orange
                                "0xff0000" }; //red
    char *pumpMenu[] = { "OFF",   
                         "ON",   
                         "AUTO",   
                         "ERROR" };                                
    /*
     * Jazz up the basic collors
     */
    //Blynk.setProperty(3, "color", "0x01ffB0"); 
                                      
    Blynk.setProperty(5, "color", ledcolorPalette[(int)blynk_payload[16]]);  // Set LED Label to HEX colour
    Blynk.setProperty(6, "color", ledcolorPalette[(int)blynk_payload[17]]);  // Set LED Label to HEX colour
    Blynk.setProperty(7, "color", ledcolorPalette[(int)blynk_payload[18]]);  // Set LED Label to HEX colour
    Blynk.setProperty(12, "color", ledcolorPalette[(int)blynk_payload[19]]);  // Set LED Label to HEX colour
    Blynk.setProperty(13, "color", ledcolorPalette[(int)blynk_payload[20]]);  // Set LED Label to HEX colour
    Blynk.setProperty(14, "color", ledcolorPalette[(int)blynk_payload[21]]);  // Set LED Label to HEX colour
    Blynk.setProperty(15, "color", ledcolorPalette[(int)blynk_payload[22]]);  // Set LED Label to HEX colour
    Blynk.setProperty(20, "color", ledcolorPalette[(int)blynk_payload[28]]);  // Set LED Label to HEX colour
    Blynk.setProperty(21, "color", ledcolorPalette[(int)blynk_payload[30]]);  // Set LED Label to HEX colour
    
    Blynk.setProperty(16, "label", pumpMenu[(int)blynk_payload[23]]);
    Blynk.setProperty(17, "label", pumpMenu[(int)blynk_payload[24]]);
    Blynk.setProperty(18, "label", pumpMenu[(int)blynk_payload[25]]);
    Blynk.setProperty(16, "labels", "OFF", "ON", "AUTO");
    Blynk.setProperty(17, "labels", "Off", "On", "Auto");
    Blynk.setProperty(18, "labels", "Off", "On", "Auto");
   /* 
    printf("LED 1 %s ",  ledcolor[(int)blynk_payload[16]]);  // Set LED Label to HEX colour
    printf("LED 2 %s ",  ledcolor[(int)blynk_payload[17]]);  // Set LED Label to HEX colour
    printf("LED 3 %s\n", ledcolor[(int)blynk_payload[18]]);
    printf("LED 1 %s ",  ledcolor[(int)blynk_payload[19]]); 
    printf("LED 2 %s",   ledcolor[(int)blynk_payload[20]]);
    printf("LED 3 %s",   ledcolor[(int)blynk_payload[21]]);
    printf("LED 4 %s\n", ledcolor[(int)blynk_payload[22]]); 
    */
     
      
/***  SEND INFO TO BLYNK     ***/              
    Blynk.virtualWrite(V0, blynk_payload[0]);  //Press Sensor Val
    Blynk.virtualWrite(V1, blynk_payload[1]);  //Level ft                            
    Blynk.virtualWrite(V2, blynk_payload[2]);  //Gallons                        
    Blynk.virtualWrite(V3, blynk_payload[3]);  //Level %
    Blynk.virtualWrite(V4, blynk_payload[4]);  //Total GPM                 
    Blynk.virtualWrite(V5, blynk_payload[5]); //LED 1 Well 1
    Blynk.virtualWrite(V6, blynk_payload[6]); //LED 2 Well 2
    Blynk.virtualWrite(V7, blynk_payload[7]); //LED 3 Well 3
    Blynk.virtualWrite(V8, (int)blynk_payload[8]);   //FW Ver
    Blynk.virtualWrite(V9, (int)blynk_payload[9]);   //Faults
    Blynk.virtualWrite(V10,(int)blynk_payload[10]);  //Cycle Count
    Blynk.virtualWrite(V11, blynk_payload[11]); //Temperature f
    Blynk.virtualWrite(V12, blynk_payload[12]); //Float 1 Hi
    Blynk.virtualWrite(V13, blynk_payload[13]); //Float 2 90%
    Blynk.virtualWrite(V14, blynk_payload[14]); //Float 3 50%
    Blynk.virtualWrite(V15, blynk_payload[15]); //Float 4 Low
    Blynk.virtualWrite(V16, (int)blynk_payload[23]);//Run Commanded P1
    Blynk.virtualWrite(V17, (int)blynk_payload[24]);//Run Commanded P2
    Blynk.virtualWrite(V18, (int)blynk_payload[25]);//Run Commanded P3
    Blynk.virtualWrite(V19, blynk_payload[26]);//House Water Press
    Blynk.virtualWrite(V20, (int)blynk_payload[27]);//LED 4 Spinkler Pump
    Blynk.virtualWrite(V21, (int)blynk_payload[29]);//LED 5 Home Pres SW
    
    Blynk.run();
    tmr.run();
}

int main(int argc, char* argv[])
{
        MQTTAsync client;
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        MQTTAsync_token token;
        int rc;
        int ch;
        MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
        MQTTAsync_setCallbacks(client, NULL, connlost, msgarrvd, NULL);
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;
        conn_opts.onSuccess = onConnect;
        conn_opts.onFailure = onConnectFailure;
        conn_opts.context = client;
        if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start connect, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }

        while(true) {
       
           loop();
        }

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
