/*
 * @file       main.cpp
 * @author     Volodymyr Shymanskyy
 * @license    This project is released under the MIT License (MIT)
 * @copyright  Copyright (c) 2015 Volodymyr Shymanskyy
 * @date       Mar 2015
 * @brief
 */

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
#include "MQTTClient.h"


//#define ADDRESS     "tcp://soldier.cloudmqtt.com:15599"
//#define mqttPort 15599 
//#define mqttUser "zerlcpdf"
//#define mqttPassword  "OyHBShF_g9ya" 

#define ADDRESS     "192.168.1.154:1883"
#define CLIENTID    "Tank Blynker"
#define TOPIC       "Tank Blynk"
#define PAYLOAD     "Hello World!"
#define QOS         0
#define TIMEOUT     10000L

float blynk_payload [30];

unsigned int controlWord1 = 0;
unsigned int controlWord2 = 0;
unsigned int controlWord3 = 0;
unsigned int controlWord = 0;
unsigned int controlWordLast = 0;

volatile MQTTClient_deliveryToken deliveredtoken;

BlynkTimer tmr;
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    float* payloadptr;
    

    printf("Recv Data Arrived - Topic Name: %s Payload length: %d\n", topicName, message->payloadlen);
    
    if (message->payloadlen != 0) {
       
          //printf("Subscriber Data Payload: ");
          payloadptr = (float *)message->payload;
          for(i=0; i < (message->payloadlen/4); i++)
          {
             blynk_payload[i] = *payloadptr++ ;
             printf("%f ", blynk_payload[i]);
          }
          printf("\n");
          MQTTClient_freeMessage(&message);
          MQTTClient_free(topicName);
    }
   
   return 1;
}
MQTTClient client;

void setup_mqtt()
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    rc = MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != 0)    {
        printf("Failed to create client, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 0;
    //conn_opts.username = mqttUser;
    //conn_opts.password = mqttPassword;
    MQTTClient_setCallbacks(client, NULL,connlost, msgarrvd, delivered);
    printf("Is Client Already Connected: %d\n",MQTTClient_isConnected(client));
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    rc = MQTTClient_subscribe(client, TOPIC, QOS);
    if (rc != 0)    {
        printf("Failed to subscribe, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }


    //return rc;
}
BLYNK_WRITE(V1)
{
    printf("Got a value: %s\n", param[0].asStr());
}
BLYNK_WRITE(V16) {
    switch (param.asInt())
    {
    case 1: // Item 1
      controlWord1 = 0b00 ;
      //printf("Pump 1 OFF selected\n");
      break;
    case 2: // Item 2
      controlWord1 = 0b01 ;
      //printf("Pump 1 ON selected\n");
      break;
    case 3: // Item 3
      controlWord1 = 0b10 ;
      //printf("Pump 1 AUTO selected\n");
      break;
    default:
      printf("Unknown item selected\n");
    }
}
BLYNK_WRITE(V17) {
    switch (param.asInt())
    {
    case 1: // Item 1
      controlWord2 = 0b00 ;
      //printf("Pump 2 OFF selected\n");
      break;
    case 2: // Item 2
      controlWord2 = 0b01 ;
      //printf("Pump 2 ON selected\n");
      break;
    case 3: // Item 3
      controlWord2 = 0b10 ;
      //printf("Pump 2 Auto selected\n");
      break;
    default:
      printf("Unknown item selected\n");
    }
}
BLYNK_WRITE(V18) {
    switch (param.asInt())
    {
    case 1: // Item 1
      controlWord3 = 0b00 ;
      //printf("Pump 3 OFF selected\n");
      break;
    case 2: // Item 2
      controlWord3 = 0b01 ;
      //printf("Pump 3 ON selected\n");
      break;
    case 3: // Item 3
      controlWord3 = 0b10 ;
      //printf("Pump 3 AUTO selected\n");
      break;
    default:
      printf("Unknown item selected\n");
    }
}
void setup()
{
    printf("Connecting to Blynk: %s, %s, %d\n", serv, auth, port);
    Blynk.begin(auth, serv, port);
   /* 
    tmr.setInterval(1000, [](){
      Blynk.virtualWrite(V0, BlynkMillis()/1000);
    });
    */
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
    //parse_options(argc, argv, auth, serv, port);
    int i;
    int rc;
    float* payloadptr;
    char  topicName[10];
    char* topicNamePtr;
    int  topicLen;
    int* topicLenPtr;
    MQTTClient_message message;
    MQTTClient_message* messagePtr;
    messagePtr = &message;
    topicNamePtr = topicName;
    topicLenPtr = &topicLen;
    
    setup();
    setup_mqtt();
    
    while(true) {
       /*
       printf("calling recv\n");
       rc = MQTTClient_receive(client, &topicNamePtr, topicLenPtr, &messagePtr, 40000);
       printf("Back from recv - Topic Name: %s  topic leng: %d\n", topicNamePtr, topicLen);
       printf("Back from recv - status: %d message length: %d\n", rc, messagePtr->payloadlen);
      */
         
       //if (rc == MQTTCLIENT_SUCCESS && messagePtr->payloadlen != 0) {
          //printf("Message arrived\n");
          //printf("     topic: %s\n", topicNamePtr);
         // printf("   message: ");
         // payloadptr = (float *)messagePtr->payload;
         // for(i=0; i<messagePtr->payloadlen; i++)
         // {
         //    putchar(*payloadptr++);
         // }
         // putchar('\n');
         //for(i=0; i <= (messagePtr->payloadlen/4); i++)
         //{
         //    blynk_payload[i] = *payloadptr++ ;
             //printf("%0f ", blynk_payload[i]);
         //}
         //printf("\n");
        
         //MQTTClient_freeMessage(&messagePtr);
         //MQTTClient_free(topicName);
       //}
       
       loop();
       /*
        * Send APP Recv'd Data back to Subsriber
        */
        controlWord = controlWord1 | (controlWord2 << 2) | (controlWord3 << 4);
        //printf("controlWord: %0x\n", controlWord);
        if (controlWord != controlWordLast) {
           MQTTClient_message pubmsg = MQTTClient_message_initializer;
           MQTTClient_deliveryToken token;
   
           pubmsg.payload = &controlWord;
           pubmsg.payloadlen = 2;
           pubmsg.qos = 0;
           pubmsg.retained = 1;
           printf("sending app data to subscriber\n");
           MQTTClient_publishMessage(client, "Tank ESP", &pubmsg, &token);
           controlWordLast = controlWord ;
       }
    }
    MQTTClient_unsubscribe(client, TOPIC);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);  
    return 0;
}

