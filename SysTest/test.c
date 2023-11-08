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
Usage: ./program_name -P or -D -v -l [1,2,3] [file_name]

Options:
-P -D = Set the MQTT Client ID Production or Development
-v : Enable verbose output
-l [1,2,3] : Set test log level:
1 = Print all messages (default)
2 = Print only verify messages
3 = Print only overall test results
file_name : Path to the file containing test data

Example usage:
./program_name -v -l 2 tests.txt

Note:

Options -P or -D are required to specify witch MQTT server to attach to
all other options are optional, but file_name is required.
If no test log level is specified, all messages will be printed.
If no verbose flag is provided, only critical messages will be printed.
*/
void print_usage() {
    printf("Usage: program_name -P or -D -v -l [1,2,3] [file_name]\n");
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

int* BufferPointerArray[9];

/**
 * Initializes the BufferPointerArray with pointers to various data payloads.
 * 
 * The BufferPointerArray is an array of pointers to integer arrays, where each integer array
 * represents a different type of data payload. This function initializes the BufferPointerArray
 * by assigning each element to the corresponding data payload.
 */
void init_buffer_pointer_array() {

    BufferPointerArray[0] = (int*)irrigationSens_.data_payload;
    BufferPointerArray[1] = (int*)tankSens_.data_payload;
    BufferPointerArray[2] = (int*)houseSens_.data_payload;
    BufferPointerArray[3] = (int*)wellSens_.data_payload;
    BufferPointerArray[4] = (int*)tankMon_.data_payload;
    BufferPointerArray[5] = (int*)wellMon_.data_payload;
    BufferPointerArray[6] = (int*)irrigationMon_.data_payload;
    BufferPointerArray[7] = (int*)monitor_.data_payload;
    BufferPointerArray[8] = (int*)alert_.data_payload;
   
    return;
}

/*
 * Initialize the Interface Name Array
 */

char* InterfaceNameArray[9][21] ;


/**
 * Initializes the InterfaceNameArray with the appropriate client and monitor data variable names.
 * The InterfaceNameArray is a 2D array of strings with dimensions 9x21.
 * The first three rows contain client data variable names for Irrigation, Tank, and House.
 * The remaining six rows contain monitor data variable names for Well, Tank, WellMonitor, FlowMonitor, MonData, and AlertData.
 * Each row can hold up to 20 characters for the variable name, with the last element being reserved for the null terminator.
 */
void init_interface_name_array() {

    int i;
    int j;

    for (int i = 0; i < 9; ++i) {
       for (int j = 0; j < 21; ++j) {
           InterfaceNameArray[i][j] = NULL;
       }
    }
    
    for (i = 0; i <= 9; i++) {
        InterfaceNameArray[0][i] = irrigationsens_ClientData_var_name[i];
        InterfaceNameArray[1][i] = tanksens_ClientData_var_name[i];
        InterfaceNameArray[2][i] = housesens_ClientData_var_name[i];
        InterfaceNameArray[3][i] = wellsens_ClientData_var_name[i];
        InterfaceNameArray[4][i] = tankmon_ClientData_var_name[i];
        InterfaceNameArray[5][i] = wellmon_ClientData_var_name[i];
        InterfaceNameArray[6][i] = irrigationmon_ClientData_var_name[i];
    }  
    for (i = 0; i <= 20; i++) {
        InterfaceNameArray[7][i] = monitor_ClientData_var_name[i];
        InterfaceNameArray[8][i] = alert_ClientData_var_name[i];
    }
    return;
}

/**
 * Searches a 2D array of strings for a target string.
 * 
 * @param arr The 2D array of strings to search.
 * @param target The target string to search for.
 * @param row A pointer to an integer where the row index of the target string will be stored.
 * @param col A pointer to an integer where the column index of the target string will be stored.
 * @return 0 if the target string is found, -1 otherwise.
 */
int search_string(char* arr[][21], char* target, int* row, int* col) {
    for (int i = 0; i <= 8; i++) {
        for (int j = 0; j <= 20; j++) {
            if (arr[i][j] == NULL) {
                continue;
            }
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
    const char *mqtt_ip;
    int mqtt_port;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        }
        else if (strcmp(argv[i], "-P") == 0) {   
            mqtt_ip = PROD_MQTT_IP;
            mqtt_port = PROD_MQTT_PORT;
        }
        else if (strcmp(argv[i], "-D") == 0) {
            mqtt_ip = DEV_MQTT_IP;
            mqtt_port = DEV_MQTT_PORT;
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
    if (mqtt_ip == NULL) {
        fprintf(stderr, "Please specify either Production (-P) or Development (-D) server\n");
        return 1;
    }

    char mqtt_address[256];
    snprintf(mqtt_address, sizeof(mqtt_address), "tcp://%s:%d", mqtt_ip, mqtt_port);

    printf("MQTT Address: %s\n", mqtt_address);

    if ((rc = MQTTClient_create(&client, mqtt_address, T_CLIENTID,
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
    
    if (verbose) { log_test(verbose, log_level, 1, "Subscribing to topic: %s using QoS: %d\n", TANKSENS_TOPICID, QOS); }
    //log_test(verbose, log_level, 1, "Test: Subscribing to topic: %s for client: %s\n", TANKSENS_TOPICID, TANKSENS_CLIENTID);
    MQTTClient_subscribe(client, TANKSENS_TOPICID, QOS);

    if (verbose) { log_test(verbose, log_level, 1, "Subscribing to topic: %s using QoS: %d\n", WELLSENS_TOPICID, QOS); }
    //log_test(verbose, log_level, 1, "Test: Subscribing to topic: %s for client: %s\n", WELLSENS_TOPICID, WELLSENS_CLIENTID);
    MQTTClient_subscribe(client, WELLSENS_TOPICID, QOS);

    if (verbose) { log_test(verbose, log_level, 1, "Subscribing to topic: %s using QoS: %d\n", IRRIGATIONSENS_TOPICID, QOS); }
    //log_test(verbose, log_level, 1, "Test: Subscribing to topic: %s for client: %s\n", IRRIGATIONSENS_TOPICID, IRRIGATIONSENS_CLIENTID);
    MQTTClient_subscribe(client, IRRIGATIONSENS_TOPICID, QOS);

    if (verbose) { log_test(verbose, log_level, 1, "Subscribing to topic: %s using QoS: %d\n", MONITOR_TOPICID, QOS); }
    //log_test(verbose, log_level, 1, "Test: Subscribing to topic: %s for client: %s\n", MONITOR_TOPICID, MONITOR_CLIENTID);
    MQTTClient_subscribe(client, MONITOR_TOPICID, QOS);

    if (verbose) { log_test(verbose, log_level, 1, "Subscribing to topic: %s using QoS: %d\n", ALERT_TOPICID, QOS); }
    //log_test(verbose, log_level, 1, "Test: Subscribing to topic: %s for client: %s\n", ALERT_TOPICID, ALERT_CLIENTID);
    MQTTClient_subscribe(client, ALERT_TOPICID, QOS);

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
            case 1:
                pubmsg.payload = (void*)tankSens_.data_payload;
                pubmsg.payloadlen = sizeof(tankSens_.data_payload);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, TANKSENS_TOPICID, &pubmsg, &token);
                break;
            case 3:
                pubmsg.payload = (void*)wellSens_.data_payload;
                pubmsg.payloadlen = sizeof(wellSens_.data_payload);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, WELLSENS_TOPICID, &pubmsg, &token);
                break;
            case 2:
                pubmsg.payload = (void*)houseSens_.data_payload;
                pubmsg.payloadlen = sizeof(houseSens_.data_payload);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, HOUSESENS_TOPICID, &pubmsg, &token);
                break;
            case 0:
                pubmsg.payload = (void*)irrigationSens_.data_payload;
                pubmsg.payloadlen = sizeof(irrigationSens_.data_payload);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, IRRIGATIONSENS_TOPICID, &pubmsg, &token);
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
    MQTTClient_publishMessage(client, TANKSENS_TOPICID, &pubmsg, &token);
    MQTTClient_publishMessage(client, WELLSENS_TOPICID, &pubmsg, &token);
    MQTTClient_publishMessage(client, IRRIGATIONSENS_TOPICID, &pubmsg, &token);


    MQTTClient_unsubscribe(client, IRRIGATIONSENS_TOPICID);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}