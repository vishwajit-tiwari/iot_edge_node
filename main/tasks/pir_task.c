/**
 * @file pir_task.c
 * @brief FreeRTOS task to handle PIR sensor events.
 * 
 * This task waits for motion detection events from the PIR sensor, updates the global sensor data structure, and implements a debounce mechanism to prevent multiple triggers from a single motion event.
 * The PIR sensor is configured to trigger an interrupt on a rising edge, and the interrupt service routine sends a signal to this task via a FreeRTOS queue. The task then processes the event, updates the shared sensor data, and ensures that the system remains responsive.
 * 
 * @note The actual implementation of the PIR sensor initialization and interrupt handling is done in the pir_init() function, which is called from the main task during system startup.
 * 
 * @author Vishwajit Kumar Tiwari
 * @date 11/04/2026
 * @copyright All rights reserved (C) 2026
 */

#include <stdio.h>
#include <stdbool.h>

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "sensor.h"
#include "supervisor.h"


#define PIR_GPIO GPIO_NUM_27

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
        // .pull_up_en = GPIO_PULLUP_DISABLE,
        // .pull_down_en = GPIO_PULLDOWN_ENABLE,
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

    supervisor_task_config_t pir_config =
    {
        .id = TASK_PIR,
        .name = "PIR",
        .type = TASK_EVENT,
        .state = TASK_STATE_INIT,
        .timeout_ms = 5000,
    };

    supervisor_register_task(TASK_PIR, &pir_config);

    while (1)
    {
        supervisor_heartbeat(TASK_PIR);

        if(xQueueReceive(pir_queue_handle, &motion_detected, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            TickType_t current_time = xTaskGetTickCount();

            if(current_time - last_wake_time >= pdMS_TO_TICKS(2000)) // 2 seconds debounce
            {
                supervisor_progress(TASK_PIR);
                
                if(xSemaphoreTake(g_sensor_mutex_handle, portMAX_DELAY) != pdTRUE)
                {
                    ESP_LOGE(PIR_TAG, "Failed to take sensor mutex in PIR task");
                }

                // Update sensor data
                g_sensor_data.motion = motion_detected;
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