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
#include "../include/water.h"

/* #define SUB_TOPIC   "Formatted Sensor Data", formatted_sensor_, len=88
* payload[0] =    Pressure Sensor Value
* payload[2] =    Water Height
* payload[3] =    Tank Gallons
* payload[4] =    Tank Percent Full
* payload[5] =    Current Sensor  1 Value
* payload[6] =    Current Sensor  2 Value
* payload[7] =    Current Sensor  3 Value
* payload[8] =    Current Sensor  4 Value
* payload[9] =    Firmware Version of ESP
* payload[10] =    I2C Fault Count
* payload[11] =    Cycle Count
* payload[12] =    Ambient Temperature
* payload[13] =    Float State 1
* payload[14] =    Float State 2
* payload[15] =    Float State 3
* payload[16] =    Float State 4
* payload[17] =    Pressure Switch State
* payload[18] =    House Water Pressure Value
* payload[19] =     spare
* payload[20] =     spare
* payload[21] =     spare
*/

/* #define SUB_TOPIC  "Monitor Data", monitor_sensor_, len=48
* payload[0] =     PumpCurrentSense[1];
* payload[2] =     PumpCurrentSense[2];
* payload[3] =     PumpCurrentSense[3];
* payload[4] =     PumpCurrentSense[4];
* payload[5] =     PumpLedColor[1];
* payload[6] =     PumpLedColor[2];
* payload[7] =     PumpLedColor[3];
* payload[8] =     PumpLedColor[4];
* payload[9] =     floatstate[1];
* payload[10] =     floatstate[2];
* payload[11] =     floatstate[3];
* payload[12] =     floatstate[4];
* payload[13] =     floatLedcolor[1];
* payload[14] =     floatLedcolor[2];
* payload[15] =     floatLedcolor[3];
* payload[16] =     floatLedcolor[4];
* payload[17] =    spare
* payload[18] =    spare
* payload[19] =    spare
* payload[20] =    Pressure LED Color
* payload[21] =    spare
* payload[22] =    spare
* payload[23] =    spare
* payload[24] =    spare
*/

/* #define SUB_TOPIC  "Alert Data"
* payload[0] =    spare
* payload[2] =    spare
* payload[3] =    spare
* payload[4] =    spare
* payload[5] =    spare
* payload[6] =    spare
* payload[7] =    spare
* payload[8] =    spare
* payload[9] =    spare
* payload[10] =    spare
* payload[11] =    spare
* payload[12] =    spare
* payload[13] =    spare
* payload[14] =    spare
* payload[15] =    spare
* payload[16] =    spare
* payload[17] =    spare
* payload[18] =    spare
* payload[19] =    spare
* payload[20] =    spare
* payload[21] =    spare
*/

#define CLIENTID        "Tank Blynker"
#define SUB_TOPIC_FORM  "Formatted Sensor Data", formatted_sensor_, len=88
#define SUB_TOPIC_MONI  "Monitor Data", monitor_sensor_, len=48
#define SUB_TOPIC_AlER  "Alert Data"

float blynk_payload [30];

MQTTClient_deliveryToken deliveredtoken;
BlynkTimer tmr;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    //printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int format_msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    time_t t;
    time(&t);
    int i;
    unsigned short int raw_data_payload[SUB_TOPIC_LEN] ;
    unsigned short int* payloadptr;
    
    
    //printf("Message From: ");
    //printf("topic: %s\n", topicName);
    
    if (message->payloadlen != 0) {
        payloadptr = message->payload;
        for(i=0; i < (message->payloadlen/2); i++)
        {
            raw_data_payload[i] = *payloadptr++ ;
            //printf("%0d ", raw_data_payload[i]);
        }
        //printf("%0X ", raw_data_payload[21]);
        //printf("%s", ctime(&t));
        printf(". ");
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        
        for ( i=0; i<=20; i++) {
            data_payload[i] = raw_data_payload[i];
        }
    }
    return 1;
}
int monitor_msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    time_t t;
    time(&t);
    int i;
    unsigned short int raw_data_payload[SUB_TOPIC_LEN] ;
    unsigned short int* payloadptr;
    
    
    //printf("Message From: ");
    //printf("topic: %s\n", topicName);
    
    if (message->payloadlen != 0) {
        payloadptr = message->payload;
        for(i=0; i < (message->payloadlen/2); i++)
        {
            raw_data_payload[i] = *payloadptr++ ;
            //printf("%0d ", raw_data_payload[i]);
        }
        //printf("%0X ", raw_data_payload[21]);
        //printf("%s", ctime(&t));
        printf(". ");
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        
        for ( i=0; i<=20; i++) {
            data_payload[i] = raw_data_payload[i];
        }
    }
    return 1;
}
int alert_msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    time_t t;
    time(&t);
    int i;
    unsigned short int raw_data_payload[SUB_TOPIC_LEN] ;
    unsigned short int* payloadptr;
    
    
    //printf("Message From: ");
    //printf("topic: %s\n", topicName);
    
    if (message->payloadlen != 0) {
        payloadptr = message->payload;
        for(i=0; i < (message->payloadlen/2); i++)
        {
            raw_data_payload[i] = *payloadptr++ ;
            //printf("%0d ", raw_data_payload[i]);
        }
        //printf("%0X ", raw_data_payload[21]);
        //printf("%s", ctime(&t));
        printf(". ");
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        
        for ( i=0; i<=20; i++) {
            data_payload[i] = raw_data_payload[i];
        }
    }
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

MQTTClient format_client;
MQTTClient monitor_client;
MQTTClient alert_client;

void setup_mqtt()
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;
    
    rc = MQTTClient_create(&format_client, ADDRESS, CLIENTID,
                           MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != 0)    {
        printf("Failed to create client, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 0;
    //conn_opts.username = mqttUser;
    //conn_opts.password = mqttPassword;
    MQTTClient_setCallbacks(format_client, NULL,connlost, form_msgarrvd, delivered);
    printf("Is Client Already Connected: %d\n",MQTTClient_isConnected(client));
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", SUB_TOPIC_FORM, CLIENTID, QOS);
    rc = MQTTClient_subscribe(format_client, SUB_TOPIC_FORM, QOS);
    if (rc != 0)    {
        printf("Failed to subscribe, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    
    rc = MQTTClient_create(&monitor_client, ADDRESS, CLIENTID,
                           MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != 0)    {
        printf("Failed to create client, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 0;
    //conn_opts.username = mqttUser;
    //conn_opts.password = mqttPassword;
    MQTTClient_setCallbacks(monitor_client, NULL,connlost, monitor_msgarrvd, delivered);
    printf("Is Client Already Connected: %d\n",MQTTClient_isConnected(client));
    if ((rc = MQTTClient_connect(monitor_client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", SUB_TOPIC_MONI, CLIENTID, QOS);
    
    rc = MQTTClient_subscribe(monitor_client, TOPIC, QOS);
    if (rc != 0)    {
        printf("Failed to subscribe, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    
    rc = MQTTClient_create(&alert_client, ADDRESS, CLIENTID,
                           MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != 0)    {
        printf("Failed to create client, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 0;
    //conn_opts.username = mqttUser;
    //conn_opts.password = mqttPassword;
    MQTTClient_setCallbacks(alert_client, NULL,connlost, alert_msgarrvd, delivered);
    printf("Is Client Already Connected: %d\n",MQTTClient_isConnected(client));
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", SUB_TOPIC_ALER, CLIENTID, QOS);
    rc = MQTTClient_subscribe(alert_client, SUB_TOPIC_ALER, QOS);
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
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    
    printf("Connecting to Blynk: %s, %s, %d\n", serv, auth, port);
    Blynk.begin(auth, serv, port);
    
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

