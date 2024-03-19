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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> // for sleep()

#define MAX_PARAMS 20

int main(int argc, char* argv[]) {
    FILE* fp;
    int data;
    char* file_name;
    bool verbose = false;
    int log_level = 3;
    char type;
    char line[256];
    // Struct to hold sensor data
    typedef struct {
        char name[20];
        int params[MAX_PARAMS];
    } SensorData;

    SensorData sensorData;
    char operation;
    int delay;
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
    while (fgets(line, sizeof(line), fp)) {

        if (line[0] == '*') {
            continue;
        } else if (line[0] == 'X') {
        // Parse extended sensor data
        operation = 'X' ;
        sscanf(line, "X %s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                sensorData.name,
                &sensorData.params[0], &sensorData.params[1], &sensorData.params[2], &sensorData.params[3],
                &sensorData.params[4], &sensorData.params[5], &sensorData.params[6], &sensorData.params[7],
                &sensorData.params[8], &sensorData.params[9], &sensorData.params[10], &sensorData.params[11],
                &sensorData.params[12], &sensorData.params[13], &sensorData.params[14], &sensorData.params[15],
                &sensorData.params[16], &sensorData.params[17], &sensorData.params[18], &sensorData.params[19]);
        } else if (line[0] == 'S') {
            // Parse sensor data
            operation = 'S' ;
            sscanf(line, "S %s %d %d %d %d %d %d %d %d %d %d",
                    sensorData.name,
                    &sensorData.params[0], &sensorData.params[1], &sensorData.params[2], &sensorData.params[3],
                    &sensorData.params[4], &sensorData.params[5], &sensorData.params[6], &sensorData.params[7],
                    &sensorData.params[8], &sensorData.params[9]);

        } else if (strncmp(line, "D Delay", 7) == 0) {
            // Parse delay time
            int delay;
            operation = 'D' ;
            sscanf(line, "D Delay %d", &delay);
        }
        else {
            log_test(verbose, log_level, 1, "Invalid operation: %c\n", operation);
            continue;
        }
        
        switch (operation) {
            case 'S':
                log_test(verbose, log_level, 1, "Set of %s\n", sensorData.name);
                printf("Setting %s\n", sensorData.name);
                pubmsg.payload = (void*)irrigationSens_.data_payload;
                pubmsg.payloadlen = sizeof(irrigationSens_.data_payload);
                pubmsg.qos = QOS;
                pubmsg.retained = 0;
                MQTTClient_publishMessage(client, IRRIGATIONSENS_TOPICID, &pubmsg, &token);
                break;
            case 'X':
                log_test(verbose, log_level, 1, "Extended Set of %s\n", sensorData.name);
                break;
            case 'D':
                log_test(verbose, log_level, 1, "Delay for %d seconds\n", delay);
                sleep(delay);
                break;
            //case '*':
                //log_test(verbose, log_level, 1, "%s\n", param);
                //break;
            default:
                log_test(verbose, log_level, 1, "Invalid operation: %c\n", operation);
                break;
        }
        /*
        * Run at this interval
        */
        sleep(1);
    }


    //log_test(verbose, log_level, 3, "Test: Ending Test:    %s  PASS COUNT:  %d   FAIL COUNT:  %d\n", file_name, pass_count, fail_count);

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