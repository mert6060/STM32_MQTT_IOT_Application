#ifndef _ESP8266_H_
#define _ESP8266_H_
#include <inttypes.h>
#include <stdbool.h>

#define RECEPTION_BUFFER_SIZE 512
extern uint8_t g_reception_buffer[RECEPTION_BUFFER_SIZE];

bool connect_to_network(const char* essid, const char *password);
bool connect_to_tcp_server(const char *ip_address, int port_number);
void send_buffer(uint8_t *buffer, uint16_t buffer_size);
void clear_reception_buffer(void);

#endif // _ESP8266_H_
