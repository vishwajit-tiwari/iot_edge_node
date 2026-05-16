/**
 * @file watchdog_task.c
 * @brief Watchdog task implementation for the IoT Edge Node project.
 * 
 * This file contains the implementation of the watchdog task, which monitors the health of other tasks in the system and takes appropriate actions if any task becomes unresponsive. The watchdog ensures that the system remains stable and can recover from potential issues.
 * The watchdog task performs the following functions:
 * 1. Initializes the watchdog mechanism.
 * 2. Provides a function for other tasks to "kick" the watchdog, indicating that they are still responsive.
 * 3. Implements the main watchdog task that periodically checks the status of all monitored tasks and takes corrective actions if necessary.
 * 
 * @note The actual implementation of the watchdog functionality is handled in this source file to maintain modularity and readability.
 * @see watchdog.h for the interface of the watchdog task.
 * 
 * @author Vishwajit Kumar Tiwari
 * @date 11/04/2026
 * @copyright All rights reserved (C) 2026
 */

#include "watchdog.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "WATCHDOG"

// Per-task timeout (must be < TWDT timeout (5s))
#define TASK_TIMEOUT_MS 3000

typedef struct
{
    TickType_t last_tick;
    uint32_t timeout_ms;
    uint32_t progress_counter;
    uint32_t last_progress;
    bool is_registered;
    fault_type_t fault; 
    uint8_t retry_count
} task_health_t;

static task_health_t g_task_health[TASK_MAX];

void watchdog_init(void)
{
    ESP_LOGI(TAG, "Dynamic watchdog startup enabled");

    for (int i = 0; i < TASK_MAX; i++)
    {
        g_task_health[i].last_tick = 0;
        g_task_health[i].timeout_ms = TASK_TIMEOUT_MS;
        g_task_health[i].progress_counter = 0;
        g_task_health[i].last_progress = 0;
        g_task_health[i].is_registered = false; // IMPORTANT
    }
}

void watchdog_register(task_id_t id)
{
    g_task_health[id].is_registered = true;
    g_task_health[id].last_tick = xTaskGetTickCount();
}

void watchdog_kick(task_id_t id)
{
    g_task_health[id].last_tick = xTaskGetTickCount();
}

void watchdog_increment_progress(task_id_t id)
{
    g_task_health[id].progress_counter++;
}

static bool check_and_classify_faults(void)
{
    TickType_t now = xTaskGetTickCount();
    bool system_ok = true;

    for (int i = 0; i < TASK_MAX; i++)
    {
        task_health_t *t = &g_task_health[i];

        // Timeout
        if ((now - t->last_tick) > pdMS_TO_TICKS(t->timeout_ms))
        {
            t->fault = FAULT_TIMEOUT;
            system_ok = false;
            continue;
        }

        // Stuck detection
        if (t->progress_counter == t->last_progress)
        {
            t->fault = FAULT_STUCK;
            system_ok = false;
            continue;
        }

        // Healthy
        t->fault = FAULT_NONE;
        t->last_progress = t->progress_counter;
    }

    return system_ok;
}

static bool attempt_recovery(int task_id)
{
    task_health_t *t = &g_task_health[task_id];

    t->retry_count++;

    ESP_LOGW(TAG, "Attempt recovery: Task %d, retry %d", task_id, t->retry_count);

    switch (task_id)
    {
        case TASK_CURRENT:
            ESP_LOGW(TAG, "Reinitializing INA219...");
            ina219_init();
            return true;

        case TASK_TEMP:
            ESP_LOGW(TAG, "Resetting temperature state...");
            return true;

        case TASK_PIR:
            ESP_LOGW(TAG, "Reinitializing PIR...");
            pir_init();
            return true;

        case TASK_AGGREGATOR:
            ESP_LOGW(TAG, "Aggregator restart not needed");
            return true;

        default:
            return false;
    }
}

static bool all_tasks_registered(void)
{
    for (int i = 0; i < TASK_MAX; i++)
    {
        if (!g_task_health[i].is_registered)
        {
            return false;
        }
    }
    return true;
}

void watchdog_task(void *arg)
{
    esp_task_wdt_add(NULL); // Add current task (watchdog) to TWDT to prevent it from being reset

    ESP_LOGI(TAG, "Waiting for all tasks to register...");

    int retry = 0;

    // Wait until all tasks have registered with the watchdog before starting monitoring
    while (!all_tasks_registered())
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        retry++;

        if (retry > 50)  // 5 seconds
        {
            ESP_LOGE(TAG, "Task registration timeout!");
            break;
        }
    }

    ESP_LOGI(TAG, "All tasks registered. Starting monitoring.");

    while (1)
    {
        bool healthy = check_and_classify_faults();

        if (healthy)
        {
            esp_task_wdt_reset();
        }
        else
        {
            bool recovered = true;

            for (int i = 0; i < TASK_MAX; i++)
            {
                task_health_t *t = &g_task_health[i];

                if (t->fault != FAULT_NONE)
                {
                    if (t->retry_count < 3)
                    {
                        recovered &= attempt_recovery(i);
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Task %d exceeded retry limit", i);
                        recovered = false;
                    }
                }
            }

            if (recovered)
            {
                ESP_LOGW(TAG, "Recovery attempted continue monitoring");
                esp_task_wdt_reset();
            }
            else
            {
                ESP_LOGE(TAG, "Critical failure not feeding watchdog");
                // Let system reset
                esp_reset_reason_t reset_reason = esp_reset_reason();
                ESP_LOGE(TAG, "System reset reason: %d", reset_reason);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
