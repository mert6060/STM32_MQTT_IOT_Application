#include "esp8266.h"
#include "stm32l4xx_hal.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart1;


uint8_t g_reception_buffer[RECEPTION_BUFFER_SIZE];
uint16_t s_reception_buffer_index = 0;

static uint8_t s_reception_byte;
static char s_dynamic_command[128] = { 0 };
static char s_dynamic_command_response[128] = { 0 };

static const char INTIAL_COMMAND[] = "AT\r\n";		     								// First Command for the testing ESP32
#define INITIAL_COMMAND_RESPONSE "AT\r\r\n\r\nOK\r\n"

static const char SET_STATION_MODE_COMMAND[] = "AT+CWMODE=1\r\n";						// Second Command for the Wi-Fi Module (Station Mode)
#define SET_STATION_MODE_COMMAND_RESPONSE "AT+CWMODE=1\r\r\n\r\nOK\r\n"

static const char DISCONNECT_FROM_WIFI_COMMAND[] = "AT+CWQAP\r\n";						// If ESP32 connect to WI-FI, disconnect from the Wi-Fi

static const char START_SINGLE_CONNECTION_COMMAND[] = "AT+CIPMUX=0\r\n";
#define START_SINGLE_CONNECTION_COMMAND_RESPONSE  "AT+CIPMUX=0\r\r\n\r\nOK\r\n"			// Enable or disable Multiple Connection

static const char ENABLE_RECEPTION_COMMNAD []  = "AT+CIPDINFO=0\r\n" ;
#define ENABLE_RECEPTION_COMMAND_RESPONSE   "AT+CIPDINFO=0\r\r\n\r\nOK\r\n"



static bool wait_for_reception(const char *expected_string, int delay_in_millisecond)
{
	HAL_Delay(delay_in_millisecond);
	int result = strcmp(expected_string, (const char*)g_reception_buffer);
	clear_reception_buffer();
	return result == 0;
}

static bool send_initial_command(void)
{
	HAL_UART_Transmit(&huart1, (const uint8_t*) INTIAL_COMMAND, sizeof(INTIAL_COMMAND), 100);
	if (wait_for_reception(INITIAL_COMMAND_RESPONSE, 1000) != true)
	{
		return false;
	}
	return true;
}

static bool send_set_station_command(void)
{
	HAL_UART_Transmit(&huart1, (const uint8_t*) SET_STATION_MODE_COMMAND, sizeof(SET_STATION_MODE_COMMAND), 100);
	if (wait_for_reception(SET_STATION_MODE_COMMAND_RESPONSE, 1000) != true)
	{
		return false;
	}
	return true;
}

static void disconnect_from_wifi(void)
{
	HAL_UART_Transmit(&huart1, (const uint8_t*) DISCONNECT_FROM_WIFI_COMMAND, sizeof(DISCONNECT_FROM_WIFI_COMMAND), 100);
	HAL_Delay(1000);
	clear_reception_buffer();
}

static bool connect_to_wifi(const char *essid, const char *password)
{
	memset(s_dynamic_command, 0 ,sizeof(s_dynamic_command));
	sprintf(s_dynamic_command, "AT+CWJAP=\"%s\",\"%s\"\r\n", essid, password);
	uint16_t dynamic_command_length = strlen(s_dynamic_command);
	HAL_UART_Transmit(&huart1, (const uint8_t*) (uint8_t*)s_dynamic_command, dynamic_command_length, 100);

	memset(s_dynamic_command_response, 0 ,sizeof(s_dynamic_command_response));
	sprintf(s_dynamic_command_response, "AT+CWJAP=\"%s\",\"%s\"\r\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n", essid, password);
	if (wait_for_reception(s_dynamic_command_response, 10000) != true)
	{
		return false;
	}
	return true;
}

static bool send_start_single_connection_command(void)
{
	HAL_UART_Transmit(&huart1, (const uint8_t*) START_SINGLE_CONNECTION_COMMAND, sizeof(START_SINGLE_CONNECTION_COMMAND), 100);
	if (wait_for_reception(START_SINGLE_CONNECTION_COMMAND_RESPONSE, 1000) != true)
	{
		return false;
	}
	return true;
}

static bool connect_to_tcp(const char *ip_address, int port_number)
{
	memset(s_dynamic_command, 0 ,sizeof(s_dynamic_command));
	sprintf(s_dynamic_command, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", ip_address, port_number);
	uint16_t dynamicCommandLength = strlen(s_dynamic_command);
	HAL_UART_Transmit(&huart1, (const uint8_t*) (uint8_t*)s_dynamic_command, dynamicCommandLength, 100);

	memset(s_dynamic_command_response, 0 ,sizeof(s_dynamic_command_response));
	sprintf(s_dynamic_command_response, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\r\nCONNECT\r\n\r\nOK\r\n", ip_address, port_number);
	if (wait_for_reception(s_dynamic_command_response, 5000) != true)
	{
		return false;
	}
	return true;
}

static bool enable_reception_from_esp()
{
	HAL_UART_Transmit(&huart1, (const uint8_t*) ENABLE_RECEPTION_COMMNAD, sizeof(ENABLE_RECEPTION_COMMNAD), 100) ;
	if (wait_for_reception(ENABLE_RECEPTION_COMMAND_RESPONSE, 2000) != true)
	{
		return false ;
	}

	return true ;
}

bool connect_to_network(const char* essid, const char *password)
{
	HAL_UART_Receive_IT(&huart1, (uint8_t*)&s_reception_byte, 1);
	HAL_Delay(1000);

	if (send_initial_command() != true)
	{
		return false;
	}
	HAL_Delay(100);

	if (send_set_station_command() != true)
	{
		return false;
	}
	HAL_Delay(100);

	disconnect_from_wifi();
	HAL_Delay(500);

	if (connect_to_wifi(essid, password) != true)
	{
		return false;
	}

	return true;
}

bool connect_to_tcp_server(const char *ip_address, int port_number)
{
	if (send_start_single_connection_command() != true)
	{
		return false;
	}
	if (connect_to_tcp(ip_address, port_number) != true)
	{
		return false;
	}
	if (enable_reception_from_esp() != true)
	{
		return false;
	}
	return true;
}

void send_buffer(uint8_t *buffer, uint16_t buffer_size)
{
	memset(s_dynamic_command, 0, sizeof(s_dynamic_command));
	sprintf(s_dynamic_command, "AT+CIPSEND=%d\r\n", buffer_size);
	uint16_t dynamic_command_length = strlen(s_dynamic_command);
	HAL_UART_Transmit(&huart1, (const uint8_t*) (uint8_t*)s_dynamic_command, dynamic_command_length, 100);
	HAL_Delay(100);

	HAL_UART_Transmit(&huart1, (const uint8_t*) (uint8_t*)buffer, buffer_size, 100);
	HAL_Delay(100);
}


void send_buffer_and_clear_response(uint8_t *buffer, uint16_t buffer_size)
{
	uint16_t start_index = s_reception_buffer_index;
	memset(s_dynamic_command, 0, sizeof(s_dynamic_command));
	sprintf(s_dynamic_command, "AT+CIPSEND=%d\r\n", buffer_size);
	uint16_t dynamic_command_length = strlen(s_dynamic_command);
	HAL_UART_Transmit(&huart1, (const uint8_t*) (uint8_t*)s_dynamic_command, dynamic_command_length, 100);
	HAL_Delay(100);

	HAL_UART_Transmit(&huart1, (const uint8_t*) (uint8_t*)buffer, buffer_size, 100);
	HAL_Delay(100);
	if (s_reception_buffer_index > start_index)
	{
		memset(&g_reception_buffer[start_index], 0, s_reception_buffer_index - start_index);
	}
	else
	{
		memset(&g_reception_buffer[start_index], 0, sizeof(g_reception_buffer) - start_index);
		memset(g_reception_buffer, 0, s_reception_buffer_index);
	}
	s_reception_buffer_index = start_index;
}


void clear_reception_buffer(void)
{
	memset(g_reception_buffer, 0, sizeof(g_reception_buffer));
	s_reception_buffer_index = 0;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	g_reception_buffer[s_reception_buffer_index] = (char) s_reception_byte;
	s_reception_buffer_index = (s_reception_buffer_index+1) % sizeof(g_reception_buffer);
	HAL_UART_Receive_IT(&huart1, (uint8_t*)&s_reception_byte, 1);
}

