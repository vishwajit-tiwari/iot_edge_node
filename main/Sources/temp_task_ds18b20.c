#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"

#include "sensor.h"
#include "temp_sensor_ds18b20.h"
#include "watchdog.h"

#define LED_BUILTIN GPIO_NUM_2 // or on board LED 

typedef enum
{
    TEMP_START,
    TEMP_WAIT,
    TEMP_READ
} temp_state_t;

void led_setup() 
{
//   pinMode(LED_BUILTIN, GPIO_MODE_OUTPUT);
  gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_BUILTIN, 0);
}

void temp_task_ds18b20(void *arg)
{
    temp_state_t state = TEMP_START;
    TickType_t start;

    led_setup();

    watchdog_register(TASK_TEMP); // Register the task with the watchdog to start monitoring

    while (1)
    {
        switch(state)
        {
            case TEMP_START:
                temp_sensor_ds18b20_start();
                start = xTaskGetTickCount();
                state = TEMP_WAIT;
                break;

            case TEMP_WAIT:
                if(xTaskGetTickCount() - start > pdMS_TO_TICKS(750))
                {
                    state = TEMP_READ;
                }

                gpio_set_level(LED_BUILTIN, 0);  // TODO: need to remove after testing
                
                break;

            case TEMP_READ:
            {
                float temp_val = temp_sensor_ds18b20_read();

                xSemaphoreTake(g_sensor_mutex_handle, portMAX_DELAY);

                if(temp_val > -55 && temp_val < 125)
                {
                    g_sensor_data.temperature = temp_val;
                    g_sensor_data.temp_valid = true;
                }
                else
                {
                    g_sensor_data.temp_valid = false;
                }

                xSemaphoreGive(g_sensor_mutex_handle);

                watchdog_kick(TASK_TEMP); // Kick the watchdog to indicate the task is still alive
                watchdog_increment_progress(TASK_TEMP); // Increment progress counter to indicate task is making progress

                gpio_set_level(LED_BUILTIN, 1);  // TODO: need to remove after testing

                state = TEMP_START;
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
}