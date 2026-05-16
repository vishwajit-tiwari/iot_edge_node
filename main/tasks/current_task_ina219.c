/**
 * @file current_task_ina219.c
 * @brief FreeRTOS task to read data from INA219 current sensor.
 * 
 * This task periodically reads voltage, current, and power values from the INA219 sensor and updates the global sensor data structure.
 * It uses a mutex to ensure thread-safe access to the shared sensor data.
 * 
 * @author Vishwajit Kumar Tiwari
 * @date 11/04/2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "sensor.h"
#include "current_sensor_ina219.h"
#include "supervisor.h"

void current_task_ina219(void *arg)
{
    float voltage, current, power;
    supervisor_task_config_t current_config =
    {
    .id = TASK_CURRENT,
    .name = "INA219",
    .type = TASK_PERIODIC,
    .state = TASK_STATE_INIT,
    .timeout_ms = 3000,
    };

    supervisor_register_task(TASK_CURRENT, &current_config);

    while (1)
    {
        bool ok = ina219_read(&voltage, &current, &power);
        // Send heartbeat to supervisor to indicate task is alive (even if read fails, we want to show it's still running)
        supervisor_heartbeat(TASK_CURRENT); 

        if (ok)
        {
            supervisor_progress(TASK_CURRENT); // Indicate progress after successful read
            xSemaphoreTake(g_sensor_mutex_handle, portMAX_DELAY);

            g_sensor_data.voltage = voltage;
            g_sensor_data.current = current;
            g_sensor_data.power = power;
            g_sensor_data.ina219_valid = true;

            xSemaphoreGive(g_sensor_mutex_handle);

        }
        else
        {
            xSemaphoreTake(g_sensor_mutex_handle, portMAX_DELAY);
            
            g_sensor_data.ina219_valid = false;

            xSemaphoreGive(g_sensor_mutex_handle);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}