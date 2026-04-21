/**
 * @file iot_edge_node: The main task for all the RTOS functionality
 * @brief This is the entry point of RTOS tasks to handle diffrent functionality:
 * ********************************************************************************
 * ------------Temperature sensing DS18B20(non-blocking state machine)-------------
 * -------------Current & Power sensing INA219 (I2C current + power)---------------
 * ------------------------PIR sensing (ISR + debounce)----------------------------
 * ********************************************************************************
 * @author Vishwajit Kumar Tiwari
 * @date 11/04/2026
 * @copyright All rights reserved (C) 2026 
 */


// header file inclusion
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/projdefs.h"

#include "sensor.h"
#include "current_sensor_ina219.h"

// tasks declaration 
extern void temp_task_ds18b20(void *arg);
extern void current_task_ina219(void *arg);
extern void pir_task(void *);
extern void data_aggregator_task(void *);
extern void watchdog_task(void *);

// sensors initialization declaration
extern void pir_init();
extern void uart_init();
extern bool ina219_init();

// gloabal declarations
sensor_data_t g_sensor_data = {0};
SemaphoreHandle_t g_sensor_mutex_handle;

// main task or entry point
void app_main(void)
{
    BaseType_t isTaskCreated = pdFALSE;

    // create semaphore for sensors data handling
    g_sensor_mutex_handle = xSemaphoreCreateMutex();
    if(g_sensor_mutex_handle == NULL)
    {
        printf("Semaphore creation failed!\n");
    }

    // call to sensors initialization
    pir_init();
    uart_init();
    ina219_init();

    /**
     * BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, 
     * const char *const pcName, 
     * const uint32_t uxStackDepth, 
     * void *const pvParameters, 
     * UBaseType_t uxPriority, 
     * TaskHandle_t *const pxCreatedTask)
     */
    isTaskCreated = xTaskCreate(temp_task_ds18b20, "Temperature Task", 2048, NULL, 3, NULL);
    if(isTaskCreated == pdFALSE)
    {
        printf("Temperature task creation failed!!\n");
    }

    isTaskCreated = xTaskCreate(current_task_ina219, "Current and Power Task", 2048, NULL, 3, NULL);
    if(isTaskCreated == pdFALSE)
    {
        printf("Current & Power task creation failed!!\n");
    }

    isTaskCreated = xTaskCreate(pir_task, "PIR Task", 2048, NULL, 4, NULL);
    if(isTaskCreated == pdFALSE)
    {
        printf("PIR task creation failed!!\n");
    }

    isTaskCreated = xTaskCreate(data_aggregator_task, "Data Aggregator Task", 2048, NULL, 2, NULL);
    if(isTaskCreated == pdFALSE)
    {
        printf("Data aggregator task creation failed!!\n");
    }

    isTaskCreated = xTaskCreate(watchdog_task, "Watchdog Task", 2048, NULL, 5, NULL);
    if(isTaskCreated == pdFALSE)
    {
        printf("Watchdog task creation failed!!\n");
    }

}
