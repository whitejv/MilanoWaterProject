#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

/* CLIENTID     "ESP CLient", #define PUB_TOPIC   "Tank ESP", tank_esp_ , len=21
* payload 0     CH1 Unused Damaged/Dead
* payload 1     CH2 Raw Sensor Current Sense Well 1 16bit
* payload 2     CH3 Raw Sensor Current Sense Well 2 16bit
* payload 3     CH4 Raw Sensor Current Sense Well 3 16bit
* payload 4     GPIO 8 bits Hex (8 bits 0-3 floats, 4 Pump Relay Command)
* payload 5     Raw Temp Celcius
* payload 6     unused
* payload 7     unused
* payload 8     CH1 4-20 mA Raw Sensor HydroStatic Pressure 16bit
* payload 9     CH2 Raw Sensor House Water Pressure 16bit ADC 0-5v
* payload 10     CH3 Unused 2 16bit
* payload 11     CH4 Raw Sensor Current Sense Irrigation pump 4 (16bit)
* payload 12     Cycle Counter 16bit Int
* payload 13     spare
* payload 14     spare
* payload 15     spare
* payload 16     I2C Panic Count 16bit Int
* payload 17     TMP100 I2C Error
* payload 18     MCP23008 I2C Error
* payload 19     MCP3428 I2C Error
* payload 20     FW Version 4 Hex
*/
/*
* payload 21     Last payload is Control Word From User
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

#define CLIENTID    "Tank Subscriber"
#define PUB_TOPIC   "Formatted Sensor Data"
#define PUB_TOPIC_LEN 88
#define SUB_TOPIC   "Tank ESP"
#define SUB_TOPIC_LEN 21


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

int main(int argc, char* argv[])
{
    
    int i;
    time_t t;
    time(&t);
    float WaterPresSensorValue;
    float PresSensorLSB = .0000625;   //lsb voltage value from datasheet
    static float PresSensorValueArray[10] = {0.,0.,0.,0.,0.,0.,0.,0.,0.,0.};
    float PresSensorValue =0;
    static int   PresIndex = 0;
    float ConstantX = .34;      //Used Excel Polynomial Fitting to come up with equation
    float Constant  = .0962;
    float PresSensorAverage = 0;
    float WaterHeight = 0;
    float TankGallons = 0;
    float TankPerFull = 0;
    float Tank_Area = 0;
    
    int Float100State = 0;
    int Float90State = 0;
    int Float50State = 0;
    int Float25State = 0;
    
    int raw_voltage1_adc = 0;
    int raw_voltage2_adc = 0;
    int raw_voltage3_adc = 0;
    int raw_voltage4_adc = 0;
    
    unsigned short int PresSensorRawValue;
    int PressSwitState;
    
    int raw_temp = 0;
    float AmbientTempF = 0;
    float AmbientTempC = 0;
    
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    
    if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID,
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
    printf("Subscribing to topic: %s\nfor client: %s using QoS: %d\n\n", SUB_TOPIC, CLIENTID, QOS);
    
    MQTTClient_subscribe(client, SUB_TOPIC, QOS);
    
    /*
     * Main Loop
     */
    
    while(1)
    {
        time(&t);
        
        /*
         * Convert Raw hydrostatic Pressure Sensor
         * A/D to Water Height, Gallons & Percent Full
         */
        
        PresSensorRawValue = data_payload[8];
        
        /*
         * Rolling Average to Smooth data
         */
        PresSensorAverage = 0;
        PresSensorValue = 0;
        PresSensorValueArray[PresIndex++] = PresSensorRawValue * PresSensorLSB;        //Convert sensor value to voltage
        PresIndex = PresIndex % 10;
        
        for( i=0; i<=9; ++i){
            PresSensorAverage += PresSensorValueArray[i];
        }
        PresSensorValue = PresSensorAverage/10;
        //printf("%d  Sensor Value: %f\n",  data_payload[9], PresSensorValue);
        
        /*
         *** Use the Equation y=Constandx(x) + Constant solve for x to compute Water Height in tank
         */
        
        if (PresSensorValue <= Constant)
        {
            PresSensorValue = Constant;
        }
        
        WaterHeight = ((PresSensorValue-ConstantX)/Constant) +.1;  //The .1 accounts for the sensor not sitting on the bottom
        //printf("Water Height = %f\n", WaterHeight);
        
        /*
         * Use the Equation (PI*R^2*WaterHeight)*VoltoGal to compute Water Gallons in tank
         */
        Tank_Area = PI * Tank_Radius_sqd ;       //area of base of tank
        TankGallons = ((Tank_Area)*WaterHeight)*VoltoGal;
        //printf("Gallons in Tank = %f\n", TankGallons);
        
        /*
         *  Use the Equation Calculated Gallons/Max Gallons to compute Percent Gallons in tank
         */
        
        TankPerFull = TankGallons/MaxTankGal * 100;
        //printf("Percent Gallons in Tank = %f\n", TankPerFull);
        
        // Channel 2 Voltage Sensor 16 bit data
        raw_voltage1_adc = (int16_t) data_payload[1];
        
        
        // Channel 3 Voltage Sensor 16 bit data
        raw_voltage2_adc = (int16_t) data_payload[2];
        //printf("voltage ch 2: %d\n", raw_voltage2_adc);
        
        
        // Channel 4 Voltage Sensor 16 bit data
        raw_voltage3_adc = (int16_t) data_payload[3];
        
        
        // MCP3428 #2 Channel 4 Voltage Sensor 16 bit data
        WaterPresSensorValue = (int16_t)data_payload[9] * .00275496;
        //printf("Raw Water Pressure: 0%x  %d  %f\n", data_payload[9],data_payload[9], WaterPresSensorValue);
        
        raw_voltage4_adc = (int16_t) data_payload[11];
        
        /*
         * Convert the Discrete data
         */
        
        Float100State  = (data_payload[4] & 0x0001) ;
        Float90State   = (data_payload[4] & 0x0002) >> 1;
        Float50State   = (data_payload[4] & 0x0004) >> 2;
        Float25State   = (data_payload[4] & 0x0008) >> 3;
        PressSwitState = (data_payload[4] & 0x0010) >> 4;
        // printf("Hi FLoat: %x  Low Float: %x\n", HighFloatState, LowFloatState) ;
        
        /*
         * Convert Raw Temp Sensor to degrees farenhiet
         */
        
        raw_temp = data_payload[5];
        AmbientTempC = raw_temp * .0625; //LSB weight for 12 bit conversion is .0625
        AmbientTempF = (AmbientTempC * 1.8) + 32.0;
        //printf("Ambient Temp:%f  \n", AmbientTempF);
        
        /*
         * Set Firmware Version
         * firmware = data_payload[20] & SubFirmware;
         */
        
        /*
         * Load Up the Data
         */
        
        formatted_sensor_payload[0] = PresSensorValue;
        formatted_sensor_payload[1] = WaterHeight;
        formatted_sensor_payload[2] = TankGallons;
        formatted_sensor_payload[3] = TankPerFull;
        formatted_sensor_payload[4] = raw_voltage1_adc;
        formatted_sensor_payload[5] = raw_voltage2_adc;
        formatted_sensor_payload[6] = raw_voltage3_adc;
        formatted_sensor_payload[7] = raw_voltage4_adc;
        formatted_sensor_payload[8] = firmware;
        formatted_sensor_payload[9] = data_payload[16];  //I2C Faults
        formatted_sensor_payload[10] = data_payload[12]; //Cycle Count
        formatted_sensor_payload[11] = AmbientTempF;
        formatted_sensor_payload[12] = Float100State;
        formatted_sensor_payload[13] = Float90State;
        formatted_sensor_payload[14] = Float50State;
        formatted_sensor_payload[15] = Float25State;
        formatted_sensor_payload[16] = PressSwitState;
        formatted_sensor_payload[17] = WaterPresSensorValue;
        
        /*
         * Load Up the Payload
         */
        for (i=0; i<=17; i++) {
            printf("%.3f ", formatted_sensor_payload[i]);
        }
        printf("%s", ctime(&t));
        
        pubmsg.payload = formatted_sensor_payload;
        pubmsg.payloadlen = PUB_TOPIC_LEN;
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        deliveredtoken = 0;
        if ((rc = MQTTClient_publishMessage(client, PUB_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
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
