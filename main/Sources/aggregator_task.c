#include "sensor.h"
#include "watchdog.h"

extern void uart_send(sensor_data_t *data);

void data_aggregator_task(void *arg)
{
    sensor_data_t copy;

    watchdog_register(TASK_AGGREGATOR); // Register the task with the watchdog to start monitoring

    while (1)
    {
        if(xSemaphoreTake(g_sensor_mutex_handle, portMAX_DELAY) != pdTRUE)
        {
            printf("Failed to take sensor mutex in data aggregator task\n");
        }

        // Copy the sensor data to a local variable
        copy = g_sensor_data;
        g_sensor_data.motion = 0; // Reset motion status after copying

        if(xSemaphoreGive(g_sensor_mutex_handle) != pdTRUE)
        {
            printf("Failed to give sensor mutex in data aggregator task\n");
        }

        // Send the copied data over UART
        uart_send(&copy);

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 seconds before the next aggregation
        
        watchdog_kick(TASK_AGGREGATOR); // Kick the watchdog to indicate the task is still alive
        watchdog_increment_progress(TASK_AGGREGATOR); // Increment progress counter to indicate task is making progress
    }
    
}