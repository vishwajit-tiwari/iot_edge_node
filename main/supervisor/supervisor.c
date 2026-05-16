/**
 * @file supervisor.c
 * @brief Implementation of a simple supervisor for monitoring FreeRTOS tasks.
 * This supervisor periodically checks the health of registered tasks based on their heartbeat and progress. If a task is deemed unhealthy (due to timeout or lack of progress), the supervisor attempts to recover it using task-specific recovery functions. The supervisor maintains a health table for each task, tracking its state, fault count, and recovery attempts.
 * The supervisor runs as a FreeRTOS task and provides APIs for tasks to register themselves, send heartbeats, and report progress. It also allows querying the current state of any registered task.
 * @author Vishwajit Kumar Tiwari
 * @date 16/05/2026
 * @copyright All rights reserved (C) 2026
 */

#include "supervisor.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "current_sensor_ina219.h"

#define TAG "SUPERVISOR"

#define SUPERVISOR_PERIOD_MS       1000
#define MAX_RECOVERY_ATTEMPTS      3


// =====================================================
// INTERNAL HEALTH TABLE
// =====================================================

typedef struct
{
    bool registered;
    TickType_t last_heartbeat;
    uint32_t progress_counter;
    uint32_t last_progress;
    supervisor_task_config_t config;
} task_health_t;

static task_health_t g_task_health[TASK_MAX];

// =====================================================
// INTERNAL HELPERS
// =====================================================

static bool is_timeout(task_health_t *task, TickType_t now)
{
    TickType_t elapsed = now - task->last_heartbeat;
    return elapsed > pdMS_TO_TICKS(task->config.timeout_ms);
}


static bool is_stuck(task_health_t *task)
{
    return task->progress_counter == task->last_progress;
}


// =====================================================
// LOCAL RECOVERY
// =====================================================

static bool recover_task(task_id_t id)
{
    switch(id)
    {
        case TASK_CURRENT:
            ESP_LOGW(TAG, "Recovering INA219");
            return ina219_recover();

        case TASK_TEMP:
            ESP_LOGW(TAG, "Temperature recovery not implemented");
            return false;

        case TASK_AGGREGATOR:
            ESP_LOGW(TAG, "Aggregator recovery");
            return true;

        case TASK_PIR:
            // PIR does not require recovery
            return true;

        default:
            return false;
    }
}


// =====================================================
// SUPERVISOR INIT
// =====================================================

void supervisor_init(void)
{
    memset(g_task_health, 0, sizeof(g_task_health));
    ESP_LOGI(TAG, "Supervisor initialized");
}


// =====================================================
// TASK REGISTRATION
// =====================================================

void supervisor_register_task(task_id_t id, supervisor_task_config_t *config)
{
    if(config == NULL)
    {
        ESP_LOGE(TAG, "NULL config");
        return;
    }

    g_task_health[id].registered = true;
    g_task_health[id].last_heartbeat = xTaskGetTickCount();

    memcpy(&g_task_health[id].config, config, sizeof(supervisor_task_config_t));

    ESP_LOGI(TAG, "Task registered [%s]", config->name);
}


// =====================================================
// HEARTBEAT
// =====================================================

void supervisor_heartbeat(task_id_t id)
{
  g_task_health[id].last_heartbeat = xTaskGetTickCount();
}


// =====================================================
// PROGRESS
// =====================================================

void supervisor_progress(task_id_t id)
{
    g_task_health[id].progress_counter++;
}


// =====================================================
// FAULT REPORT
// =====================================================

void supervisor_report_fault(task_id_t id)
{
    g_task_health[id].config.fault_count++;
    g_task_health[id].config.state = TASK_STATE_DEGRADED;
}


// =====================================================
// GET STATE
// =====================================================

task_state_t supervisor_get_state(task_id_t id)
{
    return g_task_health[id].config.state;
}


// =====================================================
// SUPERVISOR MONITOR LOOP
// =====================================================

void supervisor_task(void *arg)
{
    TickType_t now;

    while(1)
    {
        now = xTaskGetTickCount();

        for(int i = 0; i < TASK_MAX; i++)
        {
          task_health_t *task = &g_task_health[i];

          if (task->registered == false) 
          {
            continue;
          }

          bool timeout = is_timeout(task, now);
          bool stuck = false;

          // -----------------------------------------
          // PERIODIC TASKS ONLY
          // -----------------------------------------

          if (task->config.type == TASK_PERIODIC) 
          {
            stuck = is_stuck(task);
          }

            // -----------------------------------------
            // HEALTH CHECK
            // -----------------------------------------

            if(timeout || stuck)
            {
              ESP_LOGE(TAG, "Task [%s] unhealthy " "(timeout=%d stuck=%d)", task->config.name, timeout, stuck);

              task->config.state = TASK_STATE_RECOVERING;
              task->config.recovery_attempts++;

              bool recovered = recover_task(i);

              if(recovered) 
              {
                ESP_LOGI(TAG, "Task [%s] recovered", task->config.name);
                task->config.state = TASK_STATE_RUNNING;
                task->config.recovery_attempts = 0;
              } 
              else 
              {
                ESP_LOGE(TAG, "Task [%s] degraded", task->config.name);
                task->config.state = TASK_STATE_DEGRADED;
                task->config.fault_count++;
              }
            }
            else
            {
              task->config.state = TASK_STATE_RUNNING;
            }

            task->last_progress = task->progress_counter;
        }

        vTaskDelay(pdMS_TO_TICKS(SUPERVISOR_PERIOD_MS));
    }
}