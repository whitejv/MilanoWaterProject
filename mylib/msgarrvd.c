
/*
 * This function determines what message has arrived and copies the payload to the appropriate global variable.
 */

struct Payload {
  char *data;
  int len;
};

struct Payload payloads[] = {
  {(char *)flow_data_payload, FLO_LEN},
  {(char *)formatted_sensor_payload, F_LEN},
  {(char *)monitor_sensor_payload, M_LEN},
  {(char *)alert_sensor_payload, A_LEN},
  {(char *)flow_sensor_payload, FL_LEN},
  {(char *)data_payload, ESP_LEN}
};

const char *topics[] = {FLO_TOPIC, F_TOPIC, M_TOPIC, A_TOPIC, FL_TOPIC, ESP_TOPIC};

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
  /*
  printf("Message arrived:\n");
  printf("          topic: %s  ", topicName);
  printf("         length: %d  ", topicLen);
  printf("     PayloadLen: %d\n", message->payloadlen);
  printf("message: ");
  */

  for (int i = 0; i < sizeof(topics) / sizeof(topics[0]); i++) {
    if (strcmp(topicName, topics[i]) == 0) {
      struct Payload p = payloads[i];
      memcpy(p.data, message->payload, message->payloadlen);
      //for (int j = 0; j < p.len; j++) {printf("%0x ", p.data[j]);}
      //printf("\n");
      break;
    }
  }

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topicName);
}     
       
