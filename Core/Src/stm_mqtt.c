#include "stm_mqtt.h"
#include "esp8266.h"
#include <string.h>
#include "stm32l4xx_hal.h"

#define TRANSMIT_BUFFER_SIZE 128
static uint8_t s_transmit_buffer[TRANSMIT_BUFFER_SIZE];

static uint8_t s_package_identifier_count = 1;

/**
 * @brief Checks if the CONNACK message is received.
 * @retval true if CONNACK message is received, false otherwise.
 */
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

/**
 * @brief Checks if the SUBACK message with given package identifier is received.
 * @param package_identifier Package identifier to check in SUBACK message.
 * @retval true if SUBACK message with given package identifier is received, false otherwise.
 */
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

/**
 * @brief Connects to an MQTT broker.
 * @param address Pointer to the IP address string of the MQTT broker.
 * @param port Port number of the MQTT broker.
 * @param client_id Pointer to the client identifier string.
 * @param keep_alive Keep-alive interval in seconds.
 * @retval true if connection is successful, false otherwise.
 */
bool stm_mqtt_connect(const char* address, int port, const char *client_id, int keep_alive)
{
    bool result = false;
    if (connect_to_tcp_server(address, port))
    {
        memset(s_transmit_buffer, 0 , sizeof(s_transmit_buffer));
        uint8_t size = 2;
        s_transmit_buffer[0] = 0x10; // MQTT Control Packet type (CONNECT)
        s_transmit_buffer[size++] = 0x00; // Remaining length (placeholder)
        s_transmit_buffer[size++] = 0x04; // Protocol Name Length (MQTT)
        s_transmit_buffer[size++] = 'M';  // Protocol Name
        s_transmit_buffer[size++] = 'Q';
        s_transmit_buffer[size++] = 'T';
        s_transmit_buffer[size++] = 'T';
        s_transmit_buffer[size++] = 0x04; // Protocol Level (MQTT 3.1.1)
        s_transmit_buffer[size++] = 0x02; // Connect Flags (Clean Session)
        s_transmit_buffer[size++] = 0x00; // Keep Alive MSB (0 for now)
        s_transmit_buffer[size++] = keep_alive; // Keep Alive LSB (Keep-alive interval)
        s_transmit_buffer[size++] = 0x00; // Client ID Length (placeholder)

        uint8_t client_id_size = strlen(client_id);
        s_transmit_buffer[size++] = client_id_size; // Client ID Length
        memcpy(&s_transmit_buffer[size], client_id, client_id_size); // Client ID
        size += client_id_size;
        s_transmit_buffer[1] = size - 2; // Update Remaining Length field
        clear_reception_buffer();
        send_buffer(s_transmit_buffer, size);
        HAL_Delay(100); // Delay to wait for response
        if (is_connact_received())
        {
            result = true;
        }
        clear_reception_buffer();
    }
    return result;
}

/**
 * @brief Publishes a message to an MQTT topic with QoS 0.
 * @param topic Pointer to the topic string.
 * @param payload Pointer to the payload string.
 */
void stm_mqtt_publish_qos0(const char *topic, const char *payload)
{
    uint8_t topic_length = strlen(topic);
    uint8_t payload_length = strlen(payload);
    memset(s_transmit_buffer, 0, sizeof(s_transmit_buffer));
    uint8_t size = 2;
    s_transmit_buffer[0] = 0x30; // MQTT Control Packet type (PUBLISH QoS 0)
    s_transmit_buffer[size++] = 0x00; // Remaining length (placeholder)
    s_transmit_buffer[size++] = topic_length; // Topic Length
    memcpy(&s_transmit_buffer[size], topic, topic_length); // Topic
    size += topic_length;
    memcpy(&s_transmit_buffer[size], payload, payload_length); // Payload
    size += payload_length;
    s_transmit_buffer[1] = size - 2; // Update Remaining Length field
    send_buffer_and_clear_response(s_transmit_buffer, size);
}

/**
 * @brief Subscribes to an MQTT topic with QoS 0.
 * @param topic Pointer to the topic string.
 * @retval true if subscription is successful, false otherwise.
 */
bool stm_mqtt_subscribe_qos0(const char *topic)
{
    bool result = false;
    uint8_t topic_length = strlen(topic);
    memset(s_transmit_buffer, 0, sizeof(s_transmit_buffer));
    uint8_t size = 2;
    s_transmit_buffer[0] = 0x82; // MQTT Control Packet type (SUBSCRIBE QoS 0)
    s_transmit_buffer[size++] = 0x00; // Remaining length (placeholder)
    s_transmit_buffer[size++] = s_package_identifier_count++; // Package Identifier
    s_transmit_buffer[size++] = 0x00; // Topic Filter Length MSB
    s_transmit_buffer[size++] = topic_length; // Topic Filter Length LSB
    memcpy(&s_transmit_buffer[size], topic, topic_length); // Topic Filter
    size += topic_length;
    s_transmit_buffer[size++] = 0x00; // Requested QoS
    s_transmit_buffer[1] = size - 2; // Update Remaining Length field

    clear_reception_buffer();
    send_buffer(s_transmit_buffer, size);

    if (is_suback_received(s_package_identifier_count - 1))
    {
        result = true;
    }
    clear_reception_buffer();

    return result;
}

/**
 * @brief Parses the received MQTT buffer to extract topic and payload.
 * @param topic Pointer to store the extracted topic.
 * @param payload Pointer to store the extracted payload.
 * @retval true if parsing is successful, false otherwise.
 */
bool stm_mqtt_parse_received_buffer(char *topic, char *payload)
{
    for (uint16_t i = 0; i < sizeof(g_reception_buffer); i++)
    {
        if (g_reception_buffer[i] == 0x30) // Check for PUBLISH message
        {
            HAL_Delay(30); // Delay to stabilize reception
            uint8_t package_size = g_reception_buffer[(i + 1) % sizeof(g_reception_buffer)]; // Total size of the MQTT packet
            if (package_size >= 127) // Check if size is valid
            {
                clear_reception_buffer();
                return false;
            }
            package_size += 2;
            uint8_t topic_length = g_reception_buffer[(i + 3) % sizeof(g_reception_buffer)]; // Length of the topic
            if (topic_length > 127) // Check if topic length is valid
            {
                clear_reception_buffer();
                return false;
            }
            for (uint16_t j = 0; j < topic_length; j++) // Extract topic
            {
                topic[j] = g_reception_buffer[(i + 4 + j) % sizeof(g_reception_buffer)];
            }
            uint16_t payload_length = package_size - (topic_length + 4); // Calculate payload length
            for (uint16_t j = 0; j < payload_length; j++) // Extract payload
            {
                payload[j] = g_reception_buffer[(i + 4 + topic_length + j) % sizeof(g_reception_buffer)];
            }

            uint16_t remaining_bytes = sizeof(g_reception_buffer) - i;
            if (i + package_size > remaining_bytes) // Handle buffer wrap-around
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
