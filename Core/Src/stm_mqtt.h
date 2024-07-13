#ifndef _STM_MQTT_H_
#define _STM_MQTT_H_
#include <stdbool.h>

bool stm_mqtt_connect(const char* address, int port, const char *client_id, int keep_alive);
void stm_mqtt_publish_qos0(const char *topic, const char *payload);
bool stm_mqtt_subscribe_qos0(const char *topic);
bool stm_mqtt_parse_received_buffer(char *topic, char *payload);

#endif // _STM_MQTT_H_
