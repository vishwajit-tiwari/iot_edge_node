#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "sensor.h"
#include "current_sensor_ina219.h"

void current_task_ina219(void *arg)
{
    float voltage, current, power;

    while (1)
    {
        bool isSuccess = ina219_read(&voltage, &current, &power);
        if(!isSuccess)
        {
            printf("Failed to read voltage, current, and power from ina219\n");
        }

        // get semaphore to write in global sensor data structure
        const BaseType_t sem_access = xSemaphoreTake(g_sensor_mutex_handle, portMAX_DELAY);
        if(sem_access == pdFALSE)
        {
            printf("BlockTimer expired!! semaphore is busy!!\n");
        }

        if(isSuccess && sem_access)
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

        // release semaphore after use 
        xSemaphoreGive(g_sensor_mutex_handle);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
