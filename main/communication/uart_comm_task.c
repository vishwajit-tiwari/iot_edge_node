#include "driver/uart.h"
#include <stdio.h>
#include <string.h>
#include "sensor.h"
#include "uart_comm.h"
#include "crc.h"
#include "esp_log.h"

static const char *UART_TAG = "UART_COMM";

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

void uart_send(const sensor_data_t *data)
{
    if (data == NULL)
    {
        ESP_LOGE(UART_TAG, "NULL data");
        return;
    }

    char payload[128];

    int len = snprintf(payload, sizeof(payload), "<T:%.2f,I:%.2f,V:%.2f,P:%.2f,M:%d>", 
                        data->temperature, data->current, data->voltage, data->power, data->motion);
    if (len <= 0 || len >= sizeof(payload))
    {
        ESP_LOGE(UART_TAG, "Payload error");
        return;
    }

    uint16_t crc_val = 0;
    char final_payload[150];
    int final_len;

#if CRC_MODE == 16
    crc_val = crc16_compute((const uint8_t *)payload, len);
    final_len = snprintf(final_payload, sizeof(final_payload), "%s*%04X\n", payload, crc_val);
#elif CRC_MODE == 8
    crc_val = crc8_compute((const uint8_t *)payload, len);
    final_len = snprintf(final_payload, sizeof(final_payload), "%s*%02X\n", payload, crc_val);
#elif CRC_MODE == 0
    crc_val = checksum((const uint8_t *)payload, len);
    final_len = snprintf(final_payload, sizeof(final_payload), "%s*%02X\n", payload, crc_val);
#else
    #error "Invalid CRC_MODE"
#endif

    if (final_len <= 0 || final_len >= sizeof(final_payload))
    {
        ESP_LOGE(UART_TAG, "Final payload error");
        return;
    }

    int written = uart_write_bytes(UART_PORT_NUM, final_payload, final_len);

    if (written != final_len)
    {
        ESP_LOGE(UART_TAG, "Write incomplete (%d/%d)", written, final_len);
    }

#if DEBUG
    ESP_LOGI(UART_TAG, "Sent: %s", final_payload);
#endif
}

