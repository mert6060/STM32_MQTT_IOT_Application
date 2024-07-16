#ifndef _ESP8266_H_
#define _ESP8266_H_

/**
 * @file    esp8266.h
 * @brief   Header file for ESP8266 Wi-Fi module communication functions.
 */

#include <inttypes.h>
#include <stdbool.h>

#define RECEPTION_BUFFER_SIZE 512 /**< Size of reception buffer */

extern uint8_t g_reception_buffer[RECEPTION_BUFFER_SIZE]; /**< Reception buffer */

/**
 * @brief Connects to a Wi-Fi network.
 * @param essid Pointer to the ESSID (network name) string.
 * @param password Pointer to the password string for the Wi-Fi network.
 * @retval true if successfully connected, false otherwise.
 */
bool connect_to_network(const char* essid, const char *password);

/**
 * @brief Connects to a TCP server.
 * @param ip_address Pointer to the IP address string of the server.
 * @param port_number Port number of the TCP server.
 * @retval true if successfully connected, false otherwise.
 */
bool connect_to_tcp_server(const char *ip_address, int port_number);

/**
 * @brief Sends a buffer over the established connection.
 * @param buffer Pointer to the buffer containing data to be sent.
 * @param buffer_size Size of the buffer to be sent.
 */
void send_buffer(uint8_t *buffer, uint16_t buffer_size);

/**
 * @brief Clears the reception buffer.
 */
void clear_reception_buffer(void);

#endif // _ESP8266_H_
