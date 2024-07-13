#include "stm_mqtt.h"
#include "esp8266.h"
#include <string.h>
#include "stm32l4xx_hal.h"

#define TRANSMIT_BUFFER_SIZE 128
static uint8_t s_transmit_buffer[TRANSMIT_BUFFER_SIZE];

static uint8_t s_package_identifier_count = 1;

static bool is_connact_received(void)
{
    int remaining_bytes = 0;
    for (uint16_t i = 0; i < sizeof(g_reception_buffer); i++)
    {
        remaining_bytes = sizeof(g_reception_buffer) - i;
        if (g_reception_buffer[i] == 0x20)
        {
            if (remaining_bytes >= 3)
            {
                if (g_reception_buffer[i + 1] == 0x02 && g_reception_buffer[i + 2] == 0x00 && g_reception_buffer[i + 3] == 0x00)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

static bool is_suback_received(int package_identifier)
{
    int remaining_bytes = 0;
    for (uint16_t i = 0; i < sizeof(g_reception_buffer); i++)
    {
        remaining_bytes = sizeof(g_reception_buffer) - i;
        if (g_reception_buffer[i] == 0x90)
        {
            if (remaining_bytes >= 4)
            {
                if (g_reception_buffer[i + 2] == 0x00 &&
                    g_reception_buffer[i + 3] == package_identifier)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool stm_mqtt_connect(const char* address, int port, const char *client_id, int keep_alive)
{
    bool result = false;
    if (connect_to_tcp_server(address, port))
    {
        memset(s_transmit_buffer, 0 , sizeof(s_transmit_buffer));
        uint8_t size = 2;
        s_transmit_buffer[0] = 0x10;
        s_transmit_buffer[size++] = 0x00;
        s_transmit_buffer[size++] = 0x04;
        s_transmit_buffer[size++] = 'M';
        s_transmit_buffer[size++] = 'Q';
        s_transmit_buffer[size++] = 'T';
        s_transmit_buffer[size++] = 'T';
        s_transmit_buffer[size++] = 0x04;
        s_transmit_buffer[size++] = 0x02;
        s_transmit_buffer[size++] = 0x00;
        s_transmit_buffer[size++] = keep_alive;
        s_transmit_buffer[size++] = 0x00;

        uint8_t client_id_size = strlen(client_id);
        s_transmit_buffer[size++] = client_id_size;
        memcpy(&s_transmit_buffer[size], client_id, client_id_size);
        size += client_id_size;
        s_transmit_buffer [1] = size - 2;
        clear_reception_buffer();
        send_buffer(s_transmit_buffer, size);
        HAL_Delay(100);
        if (is_connact_received())
        {
            result = true;
        }
        clear_reception_buffer();
    }
    return result;
}

void stm_mqtt_publish_qos0(const char *topic, const char *payload)
{
    uint8_t topic_length = strlen(topic);
    uint8_t payload_length = strlen(payload);
    memset(s_transmit_buffer, 0, sizeof(s_transmit_buffer));
    uint8_t size = 2;
    s_transmit_buffer[0] = 0x30;
    s_transmit_buffer[size++] = 0x00;
    s_transmit_buffer[size++] = topic_length;
    memcpy(&s_transmit_buffer[size], topic, topic_length);
    size += topic_length;
    memcpy(&s_transmit_buffer[size], payload, payload_length);
    size += payload_length;
    s_transmit_buffer[1] = size - 2;
    send_buffer_and_clear_response(s_transmit_buffer, size);
}

bool stm_mqtt_subscribe_qos0(const char *topic)
{
    bool result = false;
    uint8_t topic_length = strlen(topic);
    memset(s_transmit_buffer, 0, sizeof(s_transmit_buffer));
    uint8_t size = 2;
    s_transmit_buffer[0] = 0x82;
    s_transmit_buffer[size++] = 0x00;
    s_transmit_buffer[size++] = s_package_identifier_count;
    s_transmit_buffer[size++] = 0x00;
    s_transmit_buffer[size++] = topic_length;
    memcpy(&s_transmit_buffer[size], topic, topic_length);
    size += topic_length;
    s_transmit_buffer[size++] = 0x00;
    s_transmit_buffer[1] = size - 2;

    clear_reception_buffer();
    send_buffer(s_transmit_buffer, size);

    if (is_suback_received(s_package_identifier_count))
    {
        s_package_identifier_count++;
        result = true;
    }
    clear_reception_buffer();

    return result;
}

bool stm_mqtt_parse_received_buffer(char *topic, char *payload)
{
    for (uint16_t i = 0; i < sizeof(g_reception_buffer); i++)
    {
        if (g_reception_buffer[i] == 0x30)
        {
        	HAL_Delay(30);
            uint8_t package_size = g_reception_buffer[(i + 1) % sizeof(g_reception_buffer)];
            if (package_size >= 127)
            {
                clear_reception_buffer();
                return false;
            }
            package_size += 2;
            uint8_t topic_length = g_reception_buffer[(i + 3) % sizeof(g_reception_buffer)];
            if (topic_length > 127)
            {
                clear_reception_buffer();
                return false;
            }
            for (uint16_t j = 0; j < topic_length; j++)
            {
                topic[j] = g_reception_buffer[(i + 4 + j) % sizeof(g_reception_buffer)];
            }
            uint16_t payload_length = package_size - (topic_length + 4);
            for (uint16_t j = 0; j < payload_length; j++)
            {
                payload[j] = g_reception_buffer[(i + 4 + topic_length + j) % sizeof(g_reception_buffer)];
            }

            uint16_t remaining_bytes = sizeof(g_reception_buffer) - i;
            if (i + package_size > remaining_bytes)
            {
                memset(&g_reception_buffer[i], 0, remaining_bytes);
                memset(g_reception_buffer, 0, i + package_size - remaining_bytes);
            }
            else
            {
                memset(&g_reception_buffer[i], 0, package_size);
            }
            return true;
        }
    }
    return false;
}
