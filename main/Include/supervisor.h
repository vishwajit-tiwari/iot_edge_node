/**
 * @file supervisor.h
 * @brief Supervisor module for monitoring and managing task health in an IoT edge node.
 * 
 * This module implements a supervisor mechanism that monitors the health of various tasks in the system, including temperature reading, current sensing, PIR motion detection, and data aggregation. The supervisor tracks the state of each task, detects faults such as timeouts and stuck conditions, and attempts to recover from faults when possible. It provides an interface for tasks to register themselves, send heartbeats, report progress, and report faults.
 * The supervisor runs as a separate FreeRTOS task that periodically checks the status of all registered tasks and takes appropriate actions based on their health. This ensures that the system remains stable and can recover from potential issues without requiring a full system reset.
 * 
 * @note The actual implementation of the supervisor functionality is handled in the corresponding source file (supervisor.c) to maintain modularity and readability.
 * @see supervisor.c for the implementation of the supervisor task.
 * @author Vishwajit Kumar Tiwari
 * @date 16/05/2026
 * @copyright All rights reserved (C) 2026
 */

#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    TASK_TEMP,
    TASK_CURRENT,
    TASK_PIR,
    TASK_AGGREGATOR,
    TASK_MAX
} task_id_t;

typedef enum
{
    TASK_PERIODIC,
    TASK_EVENT,
    TASK_SERVICE
} task_type_t;

typedef enum
{
    TASK_STATE_INIT,
    TASK_STATE_RUNNING,
    TASK_STATE_DEGRADED,
    TASK_STATE_RECOVERING,
    TASK_STATE_FAILED
} task_state_t;

typedef struct
{
    task_id_t id;
    const char *name;
    task_type_t type;
    task_state_t state;
    uint32_t timeout_ms;
    uint32_t fault_count;
    uint32_t recovery_attempts;

} supervisor_task_config_t;


void supervisor_init(void);
void supervisor_register_task(task_id_t id, supervisor_task_config_t *config);
void supervisor_heartbeat(task_id_t id);
void supervisor_progress(task_id_t id);
void supervisor_report_fault(task_id_t id);
task_state_t supervisor_get_state(task_id_t id);
void supervisor_task(void *arg);

#endif