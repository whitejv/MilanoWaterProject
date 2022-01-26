#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/water.h"
#include "MQTTAsync.h"

#define CLIENTID        "Tank Alert"

unsigned int recv_payload[100];

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
    float* f_payloadptr;
    int*   i_payloadptr;
    
    //printf("Message arrived:\n");
    //printf("          topic: %s  ", topicName);
    //printf("         length: %d  ", topicLen);
    //printf("     PayloadLen: %d\n", message->payloadlen);
    //printf("message: ");
 /*   
    payloadptr = (char *)message->payload;
   
    for(i=0; i<message->payloadlen; i++)
    {
        printf("%x ", *payloadptr++);
    }
    putchar('\n'); 
*/
    if (strcmp(topicName, F_TOPIC) == 0) {
       f_payloadptr = (float *)message->payload;
       for(i=0; i<F_LEN; i++)
       {
          formatted_sensor_payload[i] = *f_payloadptr++ ;
          //printf("%.3f ", formatted_sensor_payload[i]);
       }
       printf(".\n");
       printf("\n");
    }
    else if (strcmp(topicName, M_TOPIC) == 0) {
       if ( M_LEN*4 == message->payloadlen) {
          i_payloadptr = (int *)message->payload;
          for(i=0; i<M_LEN; i++)
          {
             monitor_sensor_payload[i] = *i_payloadptr++ ;
             //printf("%d ", monitor_sensor_payload[i]);
          }
          printf(": ");
          printf("\n");
      }
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
        opts.onSuccess = onSubscribe;
        opts.onFailure = onSubscribeFailure;
        opts.context = client;
        deliveredtoken = 0;
        
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", M_TOPIC, CLIENTID, QOS);
        if ((rc = MQTTAsync_subscribe(client, M_TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe Monitor Topic, return code %d\n", rc);
                exit(EXIT_FAILURE);
        }
        printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", F_TOPIC, CLIENTID, QOS);

        if ((rc = MQTTAsync_subscribe(client, F_TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
        {
                printf("Failed to start subscribe Formatted Topic, return code %d\n", rc);
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
       
          // loop();
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
