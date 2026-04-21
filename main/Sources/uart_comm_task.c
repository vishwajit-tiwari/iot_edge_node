#include "driver/uart.h"
#include <stdio.h>
#include <string.h>
#include "sensor.h"

#define UART_PORT_NUM      UART_NUM_2
#define UART_TX_PIN        17
#define UART_RX_PIN        16

static uint8_t checksum(uint8_t *data, int len) 
{
    uint8_t checkSum = 0;

    for(int i=0; i<len; i++)
    {
        checkSum ^= data[i]; 
    }

    return checkSum;
}

void uart_init(void)
{
    uart_config_t uartConf = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1
    };

    uart_driver_install(UART_PORT_NUM, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uartConf);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void uart_send(sensor_data_t *data)
{
    char payload[128];

    int len = snprintf(payload, sizeof(payload), "<T:%.2f,I:%.2f,V:%.2f,P:%.2f,M:%d>", 
                        data->temperature, data->current, data->voltage, data->power, data->motion);

    uint8_t checkSum = checksum((uint8_t *)payload, len);

    char final_payload[150];

    sprintf(final_payload, "%s*%02X\n", payload, checkSum);

    uart_write_bytes(UART_PORT_NUM, final_payload, strlen(final_payload));

    // TODO: Need to remove the debug prints after verification
    printf("Payload content: <T:%.2f, I:%.2f, V:%.2f, P:%.2f, M:%d>\n",
            data->temperature, data->current, data->voltage, data->power, data->motion);  // Debug print to verify the payload content
    printf("Checksum: %02X\n", checkSum);  // Debug print to verify the checksum value
    printf("Sent over UART: %s\n", final_payload);  // Debug print to verify the payload being sent
}
