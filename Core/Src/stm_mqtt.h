/**
 * @file    stm_mqtt.h
 * @brief   Header file for MQTT protocol implementation on STM32.
 */

#ifndef _STM_MQTT_H_
#define _STM_MQTT_H_

#include <stdbool.h>

/**
 * @brief Connects to an MQTT broker.
 * @param address Pointer to the IP address string of the MQTT broker.
 * @param port Port number of the MQTT broker.
 * @param client_id Pointer to the client identifier string.
 * @param keep_alive Keep-alive interval in seconds.
 * @retval true if connection is successful, false otherwise.
 */
bool stm_mqtt_connect(const char* address, int port, const char *client_id, int keep_alive);

/**
 * @brief Publishes a message to an MQTT topic with QoS 0.
 * @param topic Pointer to the topic string.
 * @param payload Pointer to the payload string.
 */
void stm_mqtt_publish_qos0(const char *topic, const char *payload);

/**
 * @brief Subscribes to an MQTT topic with QoS 0.
 * @param topic Pointer to the topic string.
 * @retval true if subscription is successful, false otherwise.
 */
bool stm_mqtt_subscribe_qos0(const char *topic);

/**
 * @brief Parses the received MQTT buffer to extract topic and payload.
 * @param topic Pointer to store the extracted topic.
 * @param payload Pointer to store the extracted payload.
 * @retval true if parsing is successful, false otherwise.
 */
bool stm_mqtt_parse_received_buffer(char *topic, char *payload);

#endif // _STM_MQTT_H_
