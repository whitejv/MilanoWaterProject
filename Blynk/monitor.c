#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

/* CLIENTID     "Tank Monitor",  #define PUB_TOPIC  "Monitor Data", monitor_sensor_, len=48
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


/* CLIENTID     "Tank Subscriber", #define PUB_TOPIC   "Formatted Sensor Data", formatted_sensor_, len=88
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


#define SUB_TOPIC   "Formatted Sensor Data"
#define SUB_TOPIC_LEN 21

int mon_data_payload[22] ;

float TotalDailyGallons = 0;
float TotalGPM = 0;

 
MQTTClient_deliveryToken deliveredtoken;
 
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    //printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}
 
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    time_t t;
    time(&t);
    int i;
    float  raw_data_payload[SUB_TOPIC_LEN] ;
    float* payloadptr;
      
    //printf("Message arrived:\n");
    //printf("          topic: %s  ", topicName);
    //printf("         length: %d  ", topicLen);
    //printf("     PayloadLen: %d\n", message->payloadlen);
    //printf("message: ");
    
   
    if (message->payloadlen != 0) {
         if ( F_LEN*4 == message->payloadlen) {
            payloadptr = (float *)message->payload;
            for(i=0; i < (message->payloadlen/4); i++)
            {
               raw_data_payload[i] = *payloadptr++ ;
               //printf("%0f ", raw_data_payload[i]);
             }
         }
         //printf("%0X ", raw_data_payload[21]);
         //printf("%s", ctime(&t));
         printf(".");
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        
        for ( i=0; i<=M_LEN; i++) {
            mon_data_payload[i] = (int)raw_data_payload[i];
        }
    }
    return 1;
}
 
void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}
 
int main(int argc, char* argv[])
{
    struct pump Pump[NPumps+1];     //Add 1 so that I can start at 1,2,3 not 0
    int PumpControlWord = 0 ;
    int i;
    time_t t;
    time(&t);
    int Pump1State = 0;
    int Pump2State = 0;
    int Pump3State = 0;
    int Pump4State = 0;
    int Float100State = 0;
    int Float90State = 0;
    int Float50State = 0;
    int Float25State = 0;
    int PressSwitState = 0;
    int raw_voltage1_adc = 0;
    int raw_voltage2_adc = 0;
    int raw_voltage3_adc = 0;
    int raw_voltage4_adc = 0;
    int pressState = 0;
    int pressLedColor ;
    int floatstate[5];
    int floatLedcolor[5];
    int PumpCurrentSense[5];
    int PumpLedColor[5];
    
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    
    if ((rc = MQTTClient_create(&client, ADDRESS, M_CLIENTID,
                                MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        rc = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }
    
    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        rc = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }
    
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    //conn_opts.username = mqttUser;       //only if req'd by MQTT Server
    //conn_opts.password = mqttPassword;   //only if req'd by MQTT Server
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        rc = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", SUB_TOPIC, M_CLIENTID, QOS);
    
    MQTTClient_subscribe(client, SUB_TOPIC, QOS);
    
    /*
     * Main Loop
     */
    
    while(1)
    {
        time(&t);

        // Channel 2 Voltage Sensor 16 bit data
        raw_voltage1_adc = mon_data_payload[4];
        if (raw_voltage1_adc > 2500) {
            Pump1State = 1;
            PumpCurrentSense[1] = 255;
            Pump[1].PumpPower = ON;
            PumpLedColor[1] = BLUE; }
        else {
            Pump1State =0;
            PumpCurrentSense[1] = 255 ;
            Pump[1].PumpPower = OFF;
            PumpLedColor[1] = GREEN ; }
        
        // Channel 3 Voltage Sensor 16 bit data
        raw_voltage2_adc = mon_data_payload[5];
        //printf("voltage ch 2: %d\n", raw_voltage2_adc);
        if (raw_voltage2_adc > 500){
            Pump2State = 1;
            PumpCurrentSense[2] = 255;
            Pump[2].PumpPower = ON;
            PumpLedColor[2] = BLUE;}
        else {
            Pump2State = 0;
            PumpCurrentSense[2] = 255 ;
            Pump[2].PumpPower = OFF;
            PumpLedColor[2] = GREEN ; }
        
        // Channel 4 Voltage Sensor 16 bit data
        raw_voltage3_adc = mon_data_payload[6];
        if (raw_voltage3_adc > 500){
            Pump3State = 1;
            PumpCurrentSense[3] = 255;
            Pump[3].PumpPower = ON;
            PumpLedColor[3] = BLUE;}
        else {
            Pump3State = 0;
            PumpCurrentSense[3] = 255;
            Pump[3].PumpPower = OFF;
            PumpLedColor[3] = GREEN ;}
        
        // MCP3428 #2 Channel 4 Voltage Sensor 16 bit data
        
        raw_voltage4_adc = mon_data_payload[7];
        if (raw_voltage4_adc > 500){
            Pump4State = 1;
            PumpCurrentSense[4] = 255;
            Pump[4].PumpPower = ON;
            PumpLedColor[4] = BLUE;}
        else {
            Pump4State = 0;
            PumpCurrentSense[4] = 255;
            Pump[4].PumpPower = OFF;
            PumpLedColor[4] = GREEN ;}
        
        /*
         * Convert the Discrete data
         */
        
        Float100State  = mon_data_payload[12] ;
        Float90State   = mon_data_payload[13] ;
        Float50State   = mon_data_payload[14] ;
        Float25State   = mon_data_payload[15] ;
        PressSwitState = mon_data_payload[16] ;
        
        if (Float100State == 1){
            floatstate[1] = 255;
            floatLedcolor[1] = GREEN;}
        else {
            floatstate[1] = 255;
            floatLedcolor[1] = RED;}
        
        if (Float90State == 1){
            floatstate[2] = 255;
            floatLedcolor[2] = GREEN;}
        else {
            floatstate[2] = 255;
            floatLedcolor[2] = RED; }
        
        if (Float50State == 1){
            floatstate[3] = 255;
            floatLedcolor[3] = GREEN;}
        else {
            floatstate[3] = 255;
            floatLedcolor[3] = RED; }
        
        if (Float25State == 1){
            floatstate[4] = 255;
            floatLedcolor[4] = GREEN;}
        else {
            floatstate[4] = 255;
            floatLedcolor[4] = RED; }
        
        if (PressSwitState == 1){
            pressState = 255;
            pressLedColor = BLUE;}
        else {
            pressState = 255;
            pressLedColor = GREEN; }
        
        // printf("Hi FLoat: %x  Low Float: %x\n", HighFloatState, LowFloatState) ;
        
        /*
         * Set Firmware Version
         * firmware = mon_data_payload[20] & SubFirmware;
         */
        
        /*
         * Load Up the Data
         */
        
        /* CLIENTID     "Tank Subscriber", TOPIC "Monitor Data", monitor_sensor_ */
        monitor_sensor_payload[0] =     PumpCurrentSense[1];
        monitor_sensor_payload[1] =     PumpCurrentSense[2];
        monitor_sensor_payload[2] =     PumpCurrentSense[3];
        monitor_sensor_payload[3] =     PumpCurrentSense[4];
        monitor_sensor_payload[4] =     PumpLedColor[1];
        monitor_sensor_payload[5] =     PumpLedColor[2];
        monitor_sensor_payload[6] =     PumpLedColor[3];
        monitor_sensor_payload[7] =     PumpLedColor[4];
        monitor_sensor_payload[8] =     floatstate[1];
        monitor_sensor_payload[9] =     floatstate[2];
        monitor_sensor_payload[10] =    floatstate[3];
        monitor_sensor_payload[11] =    floatstate[4];
        monitor_sensor_payload[12] =    floatLedcolor[1];
        monitor_sensor_payload[13] =    floatLedcolor[2];
        monitor_sensor_payload[14] =    floatLedcolor[3];
        monitor_sensor_payload[15] =    floatLedcolor[4];
        monitor_sensor_payload[16] =    0;
        monitor_sensor_payload[17] =    0;
        monitor_sensor_payload[18] =    pressState;
        monitor_sensor_payload[19] =    pressLedColor;

        
        /*
         * Load Up the Payload
         */
        for (i=0; i<=M_LEN; i++) {
            printf("%0x ", monitor_sensor_payload[i]);
        }
        printf("%s", ctime(&t));

        pubmsg.payload = monitor_sensor_payload;
        pubmsg.payloadlen = M_LEN * 4;
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        deliveredtoken = 0;
        if ((rc = MQTTClient_publishMessage(client, M_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
        {
            printf("Failed to publish message, return code %d\n", rc);
            rc = EXIT_FAILURE;
        }
        
        /*
         * Run at this interval
         */
        
        sleep(1) ;
    }
    
    MQTTClient_unsubscribe(client, SUB_TOPIC);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
