#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

/*
Usage: ./program_name [-v] [-r N] -l [1,2,3] [file_name]

Options:
-v : Enable verbose output
-r N : Repeat the test N times (default is 1)
-l [1,2,3] : Set test log level:
1 = Print all messages (default)
2 = Print only verify messages
3 = Print only test results
file_name : Path to the file containing test data

Example usage:
./program_name -v -r 3 -l 2 tests.txt

Note:

All options are optional, but file_name is required.
If no repeat value is specified, the test will only be run once.
If no test log level is specified, all messages will be printed.
If no verbose flag is provided, only critical messages will be printed.
*/
void print_usage() {
    printf("Usage: program_name [-v] [-r N] -l [1,2,3] [file_name]\n");
    printf("Options:\n");
    printf("  -v             : Enable verbose output\n");
    printf("  -r N           : Repeat the test N times (default is 1)\n");
    printf("  -l [1,2,3]     : Set test log level: \n");
    printf("                     1 = Print all messages (default)\n");
    printf("                     2 = Print only verify messages\n");
    printf("                     3 = Print only test results\n");
    printf("  file_name      : Path to the file containing test data\n");
}
MQTTClient_deliveryToken deliveredtoken;

void delivered(void* context, MQTTClient_deliveryToken dt)
{
    // printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

/* Using an include here to allow me to reuse a chunk of code that
   would not work as a library file. So treating it like an include to
   copy and paste the same code into multiple programs.
*/

#include "../mylib/msgarrvd.c"

void connlost(void* context, char* cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

/*
 * Initialize the Buffer Pointer Array
 */

int* BufferPointerArray[7];

void init_buffer_pointer_array() {

    BufferPointerArray[0] = (int*)tank_data_payload;
    BufferPointerArray[1] = (int*)well_data_payload;
    BufferPointerArray[2] = (int*)flow_data_payload;
    BufferPointerArray[3] = (int*)formatted_sensor_payload;
    BufferPointerArray[4] = (int*)monitor_sensor_payload;
    BufferPointerArray[5] = (int*)alert_sensor_payload;
    BufferPointerArray[6] = (int*)flow_sensor_payload;
    return;
}

/*
 * Initialize the Interface Name Array
 */

char* InterfaceNameArray[7][21];

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

int search_string(char* arr[][21], char* target, int* row, int* col) {
    for (int i = 0; i <= 6; i++) {
        for (int j = 0; j <= 20; j++) {
            if (strcmp(arr[i][j], target) == 0) {
                *row = i;
                *col = j;
                return 0;
            }
        }
    }
    return -1;
}


int main(int argc, char* argv[]) {
    FILE* fp;
    int data;
    char* file_name;
    int repeat_count = 1;
    bool verbose = false;
    int log_level = 1;
    char type;
    char operation;
    char param[18];
    static union {
        int decimal;
        float floating;
        unsigned int hex;
    } value, tol, expected = { 0 };
 
    int repeat;
    int i = 0;
    int j = 0;

    log_message("Test: Started\n");
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        }
        else if (strcmp(argv[i], "-r") == 0) {
            if (i + 1 < argc) {
                char* endptr;
                long val = strtol(argv[i + 1], &endptr, 10);
                if (*endptr == '\0' && val > 0) {
                    repeat_count = (int)val;
                    i++;
                }
                else {
                    log_message("Error: -r option expects a positive integer argument. Default is 1.\n");
                    print_usage();
                    return EXIT_FAILURE;
                }
            }
            else {
                repeat_count = 2;
            }
        }
        else if (strcmp(argv[i], "-l") == 0) {
            if (i + 1 < argc) {
                char* endptr;
                long val = strtol(argv[i + 1], &endptr, 10);
                if (*endptr == '\0' && val >= 1 && val <= 3) {
                    log_level = (int)val;
                    i++;
                }
                else {
                    log_message("Error: -l option expects 1, 2 or 3. Default is 1.\n");
                    print_usage();
                    return EXIT_FAILURE;
                }
            }
            else {
                log_level = 1;
            }
        }
        else if (file_name == NULL) {
            file_name = argv[i];
        }
        else {
            log_message("Error: too many arguments.\n");
            print_usage();
            return EXIT_FAILURE;
        }
    }

    // Check if file name is provided
    if (file_name == NULL) {
        log_message("Error: file name not provided.\n");
        print_usage();
        return EXIT_FAILURE;
    }

    // Make Sure that we can Open the file
    fp = fopen(file_name, "r");
    if (fp == NULL) {
        log_message("Error: cannot open file '%s'.\n", file_name);
        return EXIT_FAILURE;
    }
    fclose(fp); // Close the file for now
    // Print verbose messages
    if (verbose) {
        printf("Verbose Mode Enabled\n");
        printf("Repeat option: %d\n", repeat_count);
        printf("Test log level: %d\n", log_level);
        printf("File name: %s\n", file_name);
    }
    /*
    * Initialize the MQTT Messages
    */
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    if ((rc = MQTTClient_create(&client, ADDRESS, T_CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        log_message("Test: Error == Failed to Create Client. Return Code: %d\n", rc);
        if (verbose) { printf("Failed to create client, return code %d\n", rc); }
        rc = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }

    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
        log_message("Test: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
        if (verbose) { printf("Failed to set callbacks, return code %d\n", rc); }
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
        if (verbose) { printf("Failed to connect, return code %d\n", rc); }
        rc = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }

    if (verbose) { printf("Subscribing to topic: %s using QoS: %d\n", FL_TOPIC, QOS); }
    //log_message("Test: Subscribing to topic: %s for client: %s\n", FL_TOPIC, FL_CLIENTID);
    MQTTClient_subscribe(client, FL_TOPIC, QOS);

    if (verbose) { printf("Subscribing to topic: %s using QoS: %d\n", F_TOPIC, QOS); }
    //log_message("Test: Subscribing to topic: %s for client: %s\n", F_TOPIC, F_CLIENTID);
    MQTTClient_subscribe(client, F_TOPIC, QOS);

    if (verbose) { printf("Subscribing to topic: %s using QoS: %d\n", M_TOPIC, QOS); }
    //log_message("Test: Subscribing to topic: %s for client: %s\n", M_TOPIC, M_CLIENTID);
    MQTTClient_subscribe(client, M_TOPIC, QOS);

    if (verbose) { printf("Subscribing to topic: %s using QoS: %d\n", A_TOPIC, QOS); }
    //log_message("Test: Subscribing to topic: %s for client: %s\n", A_TOPIC, A_CLIENTID);
    MQTTClient_subscribe(client, A_TOPIC, QOS);

    /*
     * Initialize the Buffer Pointer Array and Name Array Before we Start
     */

    init_buffer_pointer_array();

    init_interface_name_array();

    /*
     * Main Loop
     */

    log_message("Test: Entering Main Loop -- Test File: %s\n", file_name);

    while (repeat_count--) {
        fp = fopen(file_name, "r");
        char line[80];
        while (fgets(line, 80, fp) != NULL) {
            value.decimal = 0;
            tol.decimal = 0;
            type = 'd';
            sscanf(line, "%c %s %c %d %d?", &operation, param, &type, &value.decimal, &tol.decimal );
            if (type == 'x') { // hex
                value.hex = (unsigned int)value.decimal;
                tol.hex = (unsigned int)tol.decimal;
            }
            else if (type == 'f') { // float
                value.floating = (float)value.decimal;
                tol.floating = (float)tol.decimal;
                printf("value.floating = %f, tol.floating = %f\n", value.floating, tol.floating );
            }
            else if (type != 'd') {
                if (verbose) { printf("Invalid value type: %c\n", type); }
            }
        

        switch(operation) {
            case 's':
                if (verbose) { printf("Setting %s to %d\n", param, value.decimal); }
               
                if (search_string(InterfaceNameArray, param, &i, &j) == -1) {
                   printf("Interface not found: %s\n", param);
                   return 1;
                }
                if (verbose) {printf("i = %d, j = %d \n", i, j);} 
                memcpy(&BufferPointerArray[i][j], &value, sizeof(value));
    
                switch(i) {
                    case 0:
                        pubmsg.payload = (void*)tank_data_payload;
                        pubmsg.payloadlen = sizeof(tank_data_payload);
                        pubmsg.qos = QOS;
                        pubmsg.retained = 0;
                        MQTTClient_publishMessage(client, TANK_TOPIC, &pubmsg, &token);
                        break;
                    case 1:
                        pubmsg.payload = (void*)well_data_payload;
                        pubmsg.payloadlen = sizeof(well_data_payload);
                        pubmsg.qos = QOS;
                        pubmsg.retained = 0;
                        MQTTClient_publishMessage(client, WELL_TOPIC, &pubmsg, &token);
                        break;
                    case 2:
                        pubmsg.payload = (void*)flow_data_payload;
                        pubmsg.payloadlen = sizeof(flow_data_payload);
                        pubmsg.qos = QOS;
                        pubmsg.retained = 0;
                        MQTTClient_publishMessage(client, FLO_TOPIC, &pubmsg, &token);
                        break;
                    case 3:
                        pubmsg.payload = (void*)formatted_sensor_payload;
                        pubmsg.payloadlen = sizeof(formatted_sensor_payload);
                        pubmsg.qos = QOS;
                        pubmsg.retained = 0;
                        MQTTClient_publishMessage(client, A_TOPIC, &pubmsg, &token);
                        break;
                }
                printf("The set value of %s is %d\n", param, value.decimal);
                break;
            case 'r':
                if (verbose) { printf("Reading %s\n", param); }
                search_string(InterfaceNameArray, param, &i, &j);
                if (verbose) {printf("i = %d, j = %d \n", i, j);}
                memcpy(&value, &BufferPointerArray[i][j], sizeof(value));             
                switch (type) {
                    case 'x':
                        printf("The read value of %s is 0x%04x\n", param, value.hex);
                        break;
                    case 'f':
                        printf("The read value of %s is %f\n", param, value.floating);
                        break;
                    case 'd':
                        printf("The read value of %s is %d\n", param, value.decimal);
                        break;
                }
                break;
            case 'v':
                if (verbose) { printf("Verifying %s\n", param); }
                search_string(InterfaceNameArray, param, &i, &j);
                if (verbose) {printf("i = %d, j = %d \n", i, j);}
                expected.floating = value.floating;
                memcpy(&value, &BufferPointerArray[i][j], sizeof(value));             
                switch (type) {
                    case 'x':
                        printf("The read value of %s is 0x%04x\n", param, value.hex);
                        break;
                    case 'f':
                        if (abs(value.floating - expected.floating) > tol.floating) {
                            printf("Verify: %s is %f, equal to expected %f  (tol=%f)    FAILED\n", param, value.floating, expected.floating, tol.floating);
                            return 1;
                        }
                        else {
                            printf("Verify: %s is %f, equal to expected %f   (tol=%f)   PASSED\n", param, value.floating, expected.floating, tol.floating);
                        }
                        break;
                    case 'd':
                        printf("The read value of %s is %d\n", param, value.decimal);
                        break;
                }
                break;   
            case 'c':
                if (verbose) { printf("Comparing %s\n", param); }
                break;
            case 'w':
                if (verbose) { printf("Waiting %s to %d\n", param, value.decimal); }
               
                if (search_string(InterfaceNameArray, param, &i, &j) == -1) {
                   printf("Interface not found: %s\n", param);
                   return 1;
                }
                if (verbose) {printf("i = %d, j = %d \n", i, j);}
                sleep(value.decimal); 
                break;
            case '*':
                if (verbose) { printf("%s\n", param); }
                break; 
            default:
                if (verbose) { printf("Invalid operation: %c\n", operation); }
                break;
        }
        sleep(1);
        }

        /*
         * Run at this interval
         */

       
    }
    log_message("Test: Exiting Main Loop\n");
    MQTTClient_unsubscribe(client, F_TOPIC);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}