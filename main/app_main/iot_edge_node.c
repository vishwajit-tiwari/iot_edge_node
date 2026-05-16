/**
 * @file iot_edge_node.c
 * @brief Main task for IoT edge node handling multiple sensors and data aggregation.
 * 
 * This file contains the implementation of the main task for an IoT edge node that manages multiple sensors including a DS18B20 temperature sensor, an INA219 current sensor, and a PIR motion sensor. The main task initializes the sensors, creates FreeRTOS tasks for each sensor, and handles data aggregation and transmission.
 * 
 * The main task performs the following functions:
 * 1. Initializes the I2C driver for the INA219 sensor.
 * 2. Initializes the PIR sensor and UART for data transmission.
 * 3. Creates FreeRTOS tasks for reading temperature, current, and motion data.
 * 4. Creates a data aggregator task to compile sensor data and transmit it over UART.
 * 5. Creates a watchdog task to monitor the health of the system.
 * 
 * The main task ensures that all sensor data is collected and transmitted efficiently while maintaining system stability through the watchdog mechanism.
 * 
 * @note The actual implementation of the sensor reading tasks, data aggregation, and watchdog functionality is handled in separate source files to maintain modularity and readability.
 * 
 * @see current_task_ina219.c for INA219 sensor reading task implementation.
 * @see temp_task_ds18b20.c for DS18B20 sensor reading task implementation
 * @see pir_task.c for PIR sensor reading task implementation
 * @see data_aggregator_task.c for data aggregation and transmission implementation
 * @see watchdog_task.c for system monitoring implementation
 * 
 * @author Vishwajit Kumar Tiwari
 * @date 11/04/2026
 * 
 * @copyright All rights reserved (C) 2026
 * 
 */

// header file inclusion
#include <stdio.h>
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/projdefs.h"

#include "sensor.h"
#include "current_sensor_ina219.h"
#include "i2c_manager.h"
#include "watchdog.h"

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
        ESP_LOGE("MAIN", "Semaphore creation failed!");
    }

    // call to sensors initialization
    i2c_master_init_once(); // Initialize I2C driver before INA219
    pir_init(); // Initialize PIR sensor
    uart_init(); // Initialize UART for data transmission
    ina219_init(); // Initialize INA219 current sensor

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
        ESP_LOGE("MAIN", "Temperature task creation failed!");
    }

    isTaskCreated = xTaskCreate(current_task_ina219, "Current and Power Task", 3072, NULL, 3, NULL);
    if(isTaskCreated == pdFALSE)
    {
        ESP_LOGE("MAIN", "Current and Power task creation failed!");
    }

    isTaskCreated = xTaskCreate(pir_task, "PIR Task", 2048, NULL, 4, NULL);
    if(isTaskCreated == pdFALSE)
    {
        ESP_LOGE("MAIN", "PIR task creation failed!");
    }

    isTaskCreated = xTaskCreate(data_aggregator_task, "Data Aggregator Task", 3072, NULL, 2, NULL);
    if(isTaskCreated == pdFALSE)
    {
        ESP_LOGE("MAIN", "Data aggregator task creation failed!");
    }

    isTaskCreated = xTaskCreate(watchdog_task, "Watchdog Task", 2048, NULL, 4, NULL);
    if(isTaskCreated == pdFALSE)
    {
        ESP_LOGE("MAIN", "Watchdog task creation failed!");
    }

    watchdog_init(); // Initialize the watchdog after creating tasks to start monitoring

}
