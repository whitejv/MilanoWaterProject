#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

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

/*
 * Initialize the Buffer Pointer Array
 */

int *BufferPointerArray[7];

void init_buffer_pointer_array() {

   BufferPointerArray[0] = (int *)tank_data_payload;
   BufferPointerArray[1] = (int *)well_data_payload;
   BufferPointerArray[2] = (int *)flow_data_payload;
   BufferPointerArray[3] = (int *)formatted_sensor_payload ;
   BufferPointerArray[4] = (int *)monitor_sensor_payload;
   BufferPointerArray[5] = (int *)alert_sensor_payload;
   BufferPointerArray[6] = (int *)flow_sensor_payload;
   return;
}

/*
 * Initialize the Interface Name Array
 */

char *InterfaceNameArray[7][21] ;

void init_interface_name_array() {

   int i;
   for (i = 0; i <= 20; i++) {
     InterfaceNameArray[0][i] = TankClientData_var_names[i];
     InterfaceNameArray[1][i] = WellClientData_var_names[i];
     InterfaceNameArray[2][i] = FlowClientData_var_names[i];
     InterfaceNameArray[3][i] = FormSensorData_var_names[i];
     InterfaceNameArray[4][i] = MonSensorData_var_names[i];
     InterfaceNameArray[5][i] = AlertSensorData_var_names[i];
     InterfaceNameArray[6][i] = FlowMonSensorData_var_names[i];
   }
   return;
}

void search_string(char *arr[][21], char *target, int *row, int *col) {
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 20; j++) {
      if (strcmp(arr[i][j], target) == 0) {
        *row = i;
        *col = j;
        return;
      }
    }
  }
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [-v] [-r N] -l [1,2,3] [file_name]\n", argv[0]);
        return 1;
    }

    char *file_name;
    int repeat_count = 1;
    bool verbose = false;
    int log_level = 1;
    char type;
    char operation;
    char block;
    int offset;
    union {
        int decimal;
        float floating;
        unsigned int hex;
    } value;
    int repeat;
   int i = 0;
   int j = 0;
   float fvalue = 0.0;
   float * p_fvalue;

   time_t t;
   struct tm timenow;
   time(&t);

   int SecondsFromMidnight = 0;
   int PriorSecondsFromMidnight = 0;


   log_message("Test: Started\n");

   MQTTClient client;
   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   int rc;

   if ((rc = MQTTClient_create(&client, ADDRESS, T_CLIENTID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
   {
      log_message("Test: Error == Failed to Create Client. Return Code: %d\n", rc);
      if (verbose) {printf("Failed to create client, return code %d\n", rc);}
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
   {
      log_message("Test: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
      if (verbose) {printf("Failed to set callbacks, return code %d\n", rc);}
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }

   conn_opts.keepAliveInterval = 20;
   conn_opts.cleansession = 1;
   // conn_opts.username = mqttUser;       //only if req'd by MQTT Server
   // conn_opts.password = mqttPassword;   //only if req'd by MQTT Server
   if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
   {
      log_message("Test: Error == Failed to Connect. Return Code: %d\n", rc);
      if (verbose) {printf("Failed to connect, return code %d\n", rc);}
      rc = EXIT_FAILURE;
      exit(EXIT_FAILURE);
   }
   
   if (verbose) { printf ("Subscribing to topic: %s using QoS: %d\n\n", FL_TOPIC, QOS);}
   //log_message("Test: Subscribing to topic: %s for client: %s\n", FL_TOPIC, FL_CLIENTID);
   MQTTClient_subscribe(client, FL_TOPIC, QOS);
   
   if (verbose) { printf ("Subscribing to topic: %s using QoS: %d\n\n", F_TOPIC, QOS);}
   //log_message("Test: Subscribing to topic: %s for client: %s\n", F_TOPIC, F_CLIENTID);
   MQTTClient_subscribe(client, F_TOPIC, QOS);

   if (verbose) { printf ("Subscribing to topic: %s using QoS: %d\n\n", M_TOPIC, QOS);}
   //log_message("Test: Subscribing to topic: %s for client: %s\n", M_TOPIC, M_CLIENTID);
   MQTTClient_subscribe(client, M_TOPIC, QOS);

   if (verbose) { printf ("Subscribing to topic: %s using QoS: %d\n\n", A_TOPIC, QOS);}
   //log_message("Test: Subscribing to topic: %s for client: %s\n", A_TOPIC, A_CLIENTID);
   MQTTClient_subscribe(client, A_TOPIC, QOS);

/*
 * Initialize the Buffer Pointer Array Before we Start 
 */

   init_buffer_pointer_array();

/*
 * Initialize the Interface Name Array Before we Start
 */

   init_interface_name_array();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-r") == 0) {
            if (i + 1 < argc) {
                repeat_count = atoi(argv[i + 1]);
                i++;
            } else {
                if (verbose) { printf ("Error: -r option expects a numeric argument. Default is 2.\n");}
                repeat_count = 2;
            }
        } else if (strcmp(argv[i], "-l") == 0) {
            if (i + 1 < argc) {
                log_level = atoi(argv[i + 1]);
                i++;
            } else {
                if (verbose) { printf  ("Error: -l option expects 1, 2 or 3. Default is 1.\n");}
                log_level = 1;
            }
        }  else {
            file_name = argv[i];
        }
    }

    FILE *fp;
    int data;

   fp = fopen(file_name, "r"); // open file in read mode
    if (fp == NULL) {
        if (verbose) { printf  ("Error opening file.\n");}
        return 1;
    }

    while (fscanf(fp, " %c %c %c %d", &operation, &block, &type, &offset) != EOF) {
        if (type == 'd') { // decimal
            fscanf(fp, "%d", &value.decimal);
            fscanf(fp, "%d", &repeat);
            for(int i = 0; i < repeat; i++) {
                if (verbose) { printf ("%c %c %d %d\n", operation, block, offset, value.decimal);}
            }
        } else if (type == 'x') { // hex
            fscanf(fp, "%x", &value.hex);
            fscanf(fp, "%d", &repeat);
            for(int i = 0; i < repeat; i++) {
                if (verbose) { printf ("%c %c %d 0x%x\n", operation, block, offset, value.hex);}
            }
        } else if (type == 'f') { // float
            fscanf(fp, "%f", &value.floating);
            fscanf(fp, "%d", &repeat);
            for(int i = 0; i < repeat; i++) {
                if (verbose) { printf ("%c %c %d %f\n", operation, block, offset, value.floating);}
            }
        } else {
            if (verbose) { printf ("Invalid value type: %c\n", type);}
        }
    }

   // fclose(fp); // close file

   // return 0;


   /*
    * Main Loop
    */

   log_message("Test: Entering Main Loop\n");

   while (1)
   {
      time(&t);
      localtime_r(&t, &timenow);

      /*
       * Check the time and see if we passed midnight
       * if we have then reset Alert Data
       */

      SecondsFromMidnight = (timenow.tm_hour * 60 * 60) + (timenow.tm_min * 60) + timenow.tm_sec;
      if (SecondsFromMidnight < PriorSecondsFromMidnight)
      {
         /* Reset stuff here */
      }
      // printf("seconds since midnight: %d\n", SecondsFromMidnight);
      PriorSecondsFromMidnight = SecondsFromMidnight;

   search_string( InterfaceNameArray, "tank_gallons", &i, &j );
   printf("i = %d, j = %d \n", i, j);
   
   p_fvalue = (float *)(BufferPointerArray[i]+(j*4)) ;
   fvalue = *p_fvalue;
   printf ("The value of tank_gallons is %f\n", fvalue);

      //pubmsg.payload = test_sensor_payload;
      /*
      pubmsg.payloadlen = A_LEN * 4;
      pubmsg.qos = QOS;
      pubmsg.retained = 0;
      deliveredtoken = 0;
      if ((rc = MQTTClient_publishMessage(client, A_TOPIC, &pubmsg, &token)) != MQTTCLIENT_SUCCESS)
      {
         log_message("Test: Error == Failed to Publish Message. Return Code: %d\n", rc);
         if (verbose) {printf("Failed to publish message, return code %d\n", rc);}
         rc = EXIT_FAILURE;
      }
      */
      /*
       * Run at this interval
       */

      sleep(1);
   }
   log_message("Test: Exiting Main Loop\n");
   MQTTClient_unsubscribe(client, F_TOPIC);
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}  