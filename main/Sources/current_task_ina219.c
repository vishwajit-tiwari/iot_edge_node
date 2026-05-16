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
#include "watchdog.h"

void current_task_ina219(void *arg)
{
    float voltage, current, power;

    watchdog_register(TASK_CURRENT); // Register the task with the watchdog to start monitoring

    while (1)
    {
        bool ok = ina219_read(&voltage, &current, &power);

        xSemaphoreTake(g_sensor_mutex_handle, portMAX_DELAY);

        if (ok)
        {
            g_sensor_data.voltage = voltage;
            g_sensor_data.current = current;
            g_sensor_data.power = power;
            g_sensor_data.ina219_valid = true;
        }
        else
        {
            g_sensor_data.ina219_valid = false;
        }

        xSemaphoreGive(g_sensor_mutex_handle);

        watchdog_kick(TASK_CURRENT); // Kick the watchdog to indicate the task is still alive
        watchdog_increment_progress(TASK_CURRENT); // Increment progress counter to indicate task is making progress

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}