#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sensor.h"

#define PIR_GPIO 27

static QueueHandle_t pir_queue_handle;

static const char *PIR_TAG = "PIR_TASK";

static void IRAM_ATTR pir_irq(void *arg)
{
    int val = 1;

    if(xQueueSendFromISR(pir_queue_handle, &val, NULL) != pdTRUE)
    {
        ESP_LOGE(PIR_TAG, "Failed to send PIR event to queue from ISR");
    }
}

void pir_init(void)
{
    pir_queue_handle = xQueueCreate(10, sizeof(int));

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << PIR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_POSEDGE, //!< GPIO interrupt type : rising edge
    };

    esp_err_t returnType = gpio_config(&io_conf);
    if(returnType != ESP_OK)
    {
        ESP_LOGE(PIR_TAG, "Failed to configure PIR GPIO: %s", esp_err_to_name(returnType));
    }

    returnType = gpio_install_isr_service(0);
    if(returnType != ESP_OK)
    {
        ESP_LOGE(PIR_TAG, "Failed to install ISR service for PIR GPIO: %s", esp_err_to_name(returnType));
    }

    returnType = gpio_isr_handler_add(PIR_GPIO, pir_irq, NULL);
    if(returnType != ESP_OK)
    {
        ESP_LOGE(PIR_TAG, "Failed to add ISR handler for PIR GPIO: %s", esp_err_to_name(returnType));
    }
}

void pir_task(void *arg)
{
    int motion_detected;
    TickType_t last_wake_time = 0;

    while (1)
    {
        if(xQueueReceive(pir_queue_handle, &motion_detected, portMAX_DELAY) == pdTRUE)
        {
            TickType_t current_time = xTaskGetTickCount();

            if(current_time - last_wake_time >= pdMS_TO_TICKS(2000)) // 2 seconds debounce
            {
                if(xSemaphoreTake(g_sensor_mutex_handle, portMAX_DELAY) != pdTRUE)
                {
                    ESP_LOGE(PIR_TAG, "Failed to take sensor mutex in PIR task");
                }

                // Update sensor data
                g_sensor_data.motion = true;
                g_sensor_data.motion_valid = true;

                if(xSemaphoreGive(g_sensor_mutex_handle) != pdTRUE)
                {
                    ESP_LOGE(PIR_TAG, "Failed to give sensor mutex in PIR task");
                }

                last_wake_time = current_time;
            }
        }
    }
    
}