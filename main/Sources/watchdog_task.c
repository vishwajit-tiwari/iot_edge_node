#include <stdbool.h>
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void watchdog_task(void *arg)
{
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 5000, // Set the timeout to 5 seconds
        .idle_core_mask = (1 << 0) | (1 << 1), // Monitor idle tasks on both cores (0 & 1)
        .trigger_panic = true, // Trigger a panic when the watchdog times out
    };

    // Initialize the watchdog timer with a timeout of 5 seconds
    esp_task_wdt_init(&wdt_config);

    // Add the current task to the watchdog
    esp_task_wdt_add(NULL); // NULL means the current task

    while (1)
    {
        // Feed the watchdog to prevent it from resetting the system
        esp_task_wdt_reset();

        vTaskDelay(pdMS_TO_TICKS(1000)); // Feed every 1 second
    }
}
