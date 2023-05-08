#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "unistd.h"
#include "MQTTClient.h"
#include "../include/water.h"

int verbose = FALSE;

/*
Usage: ./program_name [-v] [-r N] -l [1,2,3] [file_name]

Options:
-v : Enable verbose output
-l [1,2,3] : Set test log level:
1 = Print all messages (default)
2 = Print only verify messages
3 = Print only overall test results
file_name : Path to the file containing test data

Example usage:
./program_name -v -l 2 tests.txt

Note:

All options are optional, but file_name is required.
If no test log level is specified, all messages will be printed.
If no verbose flag is provided, only critical messages will be printed.
*/
void print_usage() {
    printf("Usage: program_name [-v] [-r N] -l [1,2,3] [file_name]\n");
    printf("Options:\n");
    printf("  -v             : Enable verbose output\n");
    printf("  -l [1,2,3]     : Set test log level: \n");
    printf("                     1 = Print all messages (default)\n");
    printf("                     2 = Print only verify messages\n");
    printf("                     3 = Print only test results\n");
    printf("  file_name      : Path to the file containing test data\n");
}
MQTTClient_deliveryToken deliveredtoken;

void delivered(void* context, MQTTClient_deliveryToken dt)
{
    //log_test(verbose, log_level, 1, "Message with token value %d delivery confirmed\n", dt);
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

int* BufferPointerArray[8];

void init_buffer_pointer_array() {

    BufferPointerArray[0] = (int*)tank_data_payload;
    BufferPointerArray[1] = (int*)well_data_payload;
    BufferPointerArray[2] = (int*)flow_data_payload;
    BufferPointerArray[3] = (int*)tank_sensor_payload;
    BufferPointerArray[4] = (int*)well_sensor_payload;
    BufferPointerArray[5] = (int*)flow_sensor_payload;
    BufferPointerArray[6] = (int*)monitor_payload;
    BufferPointerArray[7] = (int*)alert_payload;
   
    return;
}

/*
 * Initialize the Interface Name Array
 */

char* InterfaceNameArray[8][21];

void init_interface_name_array() {

    int i;
    for (i = 0; i <= 20; i++) {
        InterfaceNameArray[0][i] = TankClientData_var_name[i];
        InterfaceNameArray[1][i] = WellClientData_var_name[i];
        InterfaceNameArray[2][i] = FlowClientData_var_name[i];
        InterfaceNameArray[3][i] = TankMonitorData_var_name[i];
        InterfaceNameArray[4][i] = WellMonitorData_var_name[i];
        InterfaceNameArray[6][i] = FlowMonitorData_var_name[i];
        InterfaceNameArray[5][i] = MonData_var_names[i];
        InterfaceNameArray[5][i] = AlertData_var_names[i];
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
    bool verbose = false;
    int log_level = 3;
    char type;
    char operation;
    char param[30];
    static union {
        int decimal;
        float floating;
        unsigned int hex;
    } value, tol, expected = { 0 };
    int pass_count = 0;
    int fail_count = 0;
    int i = 0;
    int j = 0;
    int cleanup[20];


    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
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
                    log_test(verbose, log_level, 1, "Error: -l option expects 1, 2 or 3. Default is 1.\n");
                    print_usage();
                    return EXIT_FAILURE;
                }
            }
            else {
                log_level = 3;
            }
        }
        else if (file_name == NULL) {
            file_name = argv[i];
        }
        else {
            log_test(verbose, log_level, 1, "Error: too many arguments.\n");
            print_usage();
            return EXIT_FAILURE;
        }
    }

    // Check if file name is provided
    if (file_name == NULL) {
        if (argc < 2) {
            log_test(verbose, log_level, 1, "Error: file name not provided.\n");
            print_usage();
            return EXIT_FAILURE;
        }

        if (argc > 2) {
            log_test(verbose, log_level, 1, "Error: too many arguments.\n");
            print_usage();
            return EXIT_FAILURE;
        }

    }

    // Make Sure that we can Open the file
    fp = fopen(file_name, "r");
    if (fp == NULL) {
        log_test(verbose, log_level, 1, "Error: cannot open file '%s'.\n", file_name);
        return EXIT_FAILURE;
    }
    fclose(fp); // Close the file for now

    log_test(verbose, log_level, 1, "Test: Started\n");
    log_test(verbose, log_level, 1, "Verbose Mode Enabled\n");
    log_test(verbose, log_level, 1, "logging level: %d\n", log_level);
    log_test(verbose, log_level, 1, "File name: %s\n", file_name);

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
        log_test(verbose, log_level, 1, "Test: Error == Failed to Create Client. Return Code: %d\n", rc);
        if (verbose) { log_test(verbose, log_level, 1, "Failed to create client, return code %d\n", rc); }
        rc = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }

    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS)
    {
        log_test(verbose, log_level, 1, "Test: Error == Failed to Set Callbacks. Return Code: %d\n", rc);
        if (verbose) { log_test(verbose, log_level, 1, "Failed to set callbacks, return code %d\n", rc); }
        rc = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    // conn_opts.username = mqttUser;       //only if req'd by MQTT Server
    // conn_opts.password = mqttPassword;   //only if req'd by MQTT Server
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        log_test(verbose, log_level, 1, "Test: Error == Failed to Connect. Return Code: %d\n", rc);
        if (verbose) { log_test(verbose, log_level, 1, "Failed to connect, return code %d\n", rc); }
        rc = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }
    
    if (verbose) { log_test(verbose, log_level, 1, "Subscribing to topic: %s using QoS: %d\n", TANK_TOPIC, QOS); }
    //log_test(verbose, log_level, 1, "Test: Subscribing to topic: %s for client: %s\n", TANK_TOPIC, FL_CLIENTID);
    MQTTClient_subscribe(client, TANK_TOPIC, QOS);

    if (verbose) { log_test(verbose, log_level, 1, "Subscribing to topic: %s using QoS: %d\n", WELL_TOPIC, QOS); }
    //log_test(verbose, log_level, 1, "Test: Subscribing to topic: %s for client: %s\n", FL_TOPIC, FL_CLIENTID);
    MQTTClient_subscribe(client, WELL_TOPIC, QOS);

    if (verbose) { log_test(verbose, log_level, 1, "Subscribing to topic: %s using QoS: %d\n", FLOW_TOPIC, QOS); }
    //log_test(verbose, log_level, 1, "Test: Subscribing to topic: %s for client: %s\n", F_TOPIC, F_CLIENTID);
    MQTTClient_subscribe(client, FLOW_TOPIC, QOS);

    if (verbose) { log_test(verbose, log_level, 1, "Subscribing to topic: %s using QoS: %d\n", M_TOPIC, QOS); }
    //log_test(verbose, log_level, 1, "Test: Subscribing to topic: %s for client: %s\n", M_TOPIC, M_CLIENTID);
    MQTTClient_subscribe(client, M_TOPIC, QOS);

    if (verbose) { log_test(verbose, log_level, 1, "Subscribing to topic: %s using QoS: %d\n", A_TOPIC, QOS); }
    //log_test(verbose, log_level, 1, "Test: Subscribing to topic: %s for client: %s\n", A_TOPIC, A_CLIENTID);
    MQTTClient_subscribe(client, A_TOPIC, QOS);

    /*
     * Initialize the Buffer Pointer Array and Name Array Before we Start
     */

    init_buffer_pointer_array();

    init_interface_name_array();

    /*
     * Main Loop
     */
    log_test(verbose, log_level, 3, "\n\n");
    log_test(verbose, log_level, 3, "Test: Beginning Test: %s\n", file_name);
    // Open the file
    fp = fopen(file_name, "r");
    if (fp == NULL) {
        log_test(verbose, log_level, 3, "Failed to open file: %s\n", file_name);
        return -1;
    }

    char line[80];
    while (fgets(line, 80, fp) != NULL) {

        int n = 0;
        if (sscanf(line, "%c %s %c%n", &operation, param, &type, &n) == 3) {

            if (operation == '*') {
                log_test(verbose, log_level, 2, "%s\n", line);
                continue;
            }

            if (type == 'f') {
                value.floating = 0;
                tol.floating = 0;
                sscanf(line + n, "%f %f?", &value.floating, &tol.floating);
            }
            else if (type == 'd') {
                value.decimal = 0;
                tol.decimal = 0;
                sscanf(line + n, "%d %d?", &value.decimal, &tol.decimal);
            }
            else if (type == 'x') {
                value.hex = 0;
                tol.hex = 0;
                sscanf(line + n, "%x %x?", &value.hex, &tol.hex);
            }
            else {
                log_test(verbose, log_level, 1, "Invalid value type: %c\n", type);
                continue;
            }
        }
        else {
            log_test(verbose, log_level, 1, "Failed to read line: %s\n", line);
            continue;
        }

        switch (operation) {
        case 's':

            log_test(verbose, log_level, 1, "Setting %s\n", param);
            if (search_string(InterfaceNameArray, param, &i, &j) == -1) {
                log_test(verbose, log_level, 1, "Interface not found: %s\n", param);
                return 1;
            }
            log_test(verbose, log_level, 1, "i = %d, j = %d \n", i, j);
            memcpy(&BufferPointerArray[i][j], &value, sizeof(value));
            switch (type) {
            case 'x':
                log_test(verbose, log_level, 1, "Setting %s to 0x%04x\n", param, value.hex);
                break;
            case 'f':
                log_test(verbose, log_level, 1, "Setting %s to %f\n", param, value.floating);
                break;
            case 'd':
                log_test(verbose, log_level, 1, "Setting %s to %d\n", param, value.decimal);
                break;
            }
            switch (i) {
            case 0:
                pubmsg.payload = (void*)tank_data_payload;
                pubmsg.payloadlen = sizeof(tank_data_payload);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, TANK_CLIENT, &pubmsg, &token);
                break;
            case 1:
                pubmsg.payload = (void*)well_data_payload;
                pubmsg.payloadlen = sizeof(well_data_payload);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, WELL_CLIENT, &pubmsg, &token);
                break;
            case 2:
                pubmsg.payload = (void*)flow_data_payload;
                pubmsg.payloadlen = sizeof(flow_data_payload);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, FLOW_CLIENT, &pubmsg, &token);
                break;
            }
            log_test(verbose, log_level, 1, "The set value of %s is %d\n", param, value.decimal);
            break;
        case 'r':
            log_test(verbose, log_level, 1, "Reading %s\n", param);
            if (search_string(InterfaceNameArray, param, &i, &j) == -1) {
                log_test(verbose, log_level, 1, "Interface not found: %s\n", param);
                return 1;
            }
            log_test(verbose, log_level, 1, "i = %d, j = %d \n", i, j);
            memcpy(&value, &BufferPointerArray[i][j], sizeof(value));
            switch (type) {
            case 'x':
                log_test(verbose, log_level, 1, "The read value of %s is 0x%04x\n", param, value.hex);
                break;
            case 'f':
                log_test(verbose, log_level, 1, "The read value of %s is %f\n", param, value.floating);
                break;
            case 'd':
                log_test(verbose, log_level, 1, "The read value of %s is %d\n", param, value.decimal);
                break;
            }
            break;
        case 'v':
            log_test(verbose, log_level, 1, "Verifying %s\n", param);
            if (search_string(InterfaceNameArray, param, &i, &j) == -1) {
                log_test(verbose, log_level, 1, "Interface not found: %s\n", param);
                return 1;
            }
            log_test(verbose, log_level, 1, "i = %d, j = %d \n", i, j);

            switch (type) {
            case 'x':
                expected.hex = value.hex;
                memcpy(&value, &BufferPointerArray[i][j], sizeof(value));
                if (abs(value.hex - expected.hex) > tol.hex) {
                    log_test(verbose, log_level, 2, "------------------------------------------------------------------------------------\n");
                    log_test(verbose, log_level, 2, "FAILED:  %s=0x%04x, is not equal to expected 0x%04x  (tol=0x%04x)\n", param, value.hex, expected.hex, tol.hex);
                    log_test(verbose, log_level, 2, "------------------------------------------------------------------------------------\n");
                    fail_count++;
                }
                else {
                    log_test(verbose, log_level, 2, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                    log_test(verbose, log_level, 2, "PASSED:  %s=0x%04x, is equal to expected 0x%04x   (tol=0x%04x)\n", param, value.hex, expected.hex, tol.hex);
                    log_test(verbose, log_level, 2, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                    pass_count++;
                }
                break;
            case 'f':
                expected.floating = value.floating;
                memcpy(&value, &BufferPointerArray[i][j], sizeof(value));
                if (abs(value.floating - expected.floating) > tol.floating) {
                    log_test(verbose, log_level, 2, "-----------------------------------------------------------------------------------\n");
                    log_test(verbose, log_level, 2, "FAILED:  %s=%f, is not equal to expected %f  (tol=%f)\n", param, value.floating, expected.floating, tol.floating);
                    log_test(verbose, log_level, 2, "-----------------------------------------------------------------------------------\n");
                    fail_count++;
                }
                else {
                    log_test(verbose, log_level, 2, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                    log_test(verbose, log_level, 2, "PASSED:  %s=%f, is equal to expected %f   (tol=%f)\n", param, value.floating, expected.floating, tol.floating);
                    log_test(verbose, log_level, 2, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                    pass_count++;
                }
                break;
            case 'd':
                expected.decimal = value.decimal;
                memcpy(&value, &BufferPointerArray[i][j], sizeof(value));
                if (abs(value.decimal - expected.decimal) > tol.decimal) {
                    log_test(verbose, log_level, 2, "------------------------------------------------------------------------------------\n");
                    log_test(verbose, log_level, 2, "FAILED:  %s=%d, is not equal to expected %d  (tol=%d)\n", param, value.decimal, expected.decimal, tol.decimal);
                    log_test(verbose, log_level, 2, "------------------------------------------------------------------------------------\n");
                    fail_count++;
                }
                else {
                    log_test(verbose, log_level, 2, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                    log_test(verbose, log_level, 2, "PASSED:  %s=%d, is equal to expected %d   (tol=%d)\n", param, value.decimal, expected.decimal, tol.decimal);
                    log_test(verbose, log_level, 2, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                    pass_count++;
                }
                break;
            }
            break;
        case 'c':
            log_test(verbose, log_level, 1, "Comparing %s\n", param);
            break;
        case 'd':
            log_test(verbose, log_level, 1, "%s for %d seconds\n", param, value.decimal);

            if (search_string(InterfaceNameArray, param, &i, &j) == -1) {
                log_test(verbose, log_level, 1, "Interface not found: %s\n", param);
                return 1;
            }
            log_test(verbose, log_level, 1, "i = %d, j = %d \n", i, j);
            sleep(value.decimal);
            break;
        case '*':
            log_test(verbose, log_level, 1, "%s\n", param);
            break;
        default:
            log_test(verbose, log_level, 1, "Invalid operation: %c\n", operation);
            break;
        }

        /*
         * Run at this interval
         */
        sleep(2);

    }
    log_test(verbose, log_level, 3, "Test: Ending Test:    %s  PASS COUNT:  %d   FAIL COUNT:  %d\n", file_name, pass_count, fail_count);

    /*
     * Clean up the memory after test is complete
     */
    memset(cleanup, 0, sizeof(cleanup));
    pubmsg.payload = (void*)cleanup;
    pubmsg.payloadlen = sizeof(cleanup);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, TANK_CLIENT, &pubmsg, &token);
    MQTTClient_publishMessage(client, WELL_CLIENT, &pubmsg, &token);
    MQTTClient_publishMessage(client, FLOW_CLIENT, &pubmsg, &token);


    MQTTClient_unsubscribe(client, FLOW_TOPIC);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}