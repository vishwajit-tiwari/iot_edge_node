/**
 * @file watchdog.h
 * @brief Watchdog task header file for the IoT Edge Node project.
 * 
 * This header file defines the interface for the watchdog task, which monitors the health of other tasks in the system and takes appropriate actions if any task becomes unresponsive. The watchdog ensures that the system remains stable and can recover from potential issues.
 * The watchdog task performs the following functions:
 * 1. Initializes the watchdog mechanism.
 * 2. Provides a function for other tasks to "kick" the watchdog, indicating that they are still responsive.
 * 3. Implements the main watchdog task that periodically checks the status of all monitored tasks and takes corrective actions if necessary.
 * 
 * @note The actual implementation of the watchdog functionality is handled in the corresponding source file (watchdog_task.c) to maintain modularity and readability.
 * @see watchdog_task.c for the implementation of the watchdog task.
 * 
 * @author Vishwajit Kumar Tiwari
 * @date 11/04/2026
 * @copyright All rights reserved (C) 2026
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

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
    FAULT_NONE,
    FAULT_TIMEOUT,
    FAULT_STUCK,
    FAULT_SENSOR_INVALID
} fault_type_t;

void watchdog_init(void);

// Called once when task starts
void watchdog_register(task_id_t id);

// Called periodically
void watchdog_kick(task_id_t id);
void watchdog_increment_progress(task_id_t id);

void watchdog_task(void *arg);

#endif
