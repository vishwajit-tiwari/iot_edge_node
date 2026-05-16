#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct
{
    float temperature;
    float current;
    float voltage;
    float power;

    int motion;

    bool temp_valid;
    bool ina219_valid;
    bool motion_valid;

} sensor_data_t;

extern sensor_data_t g_sensor_data;
extern SemaphoreHandle_t g_sensor_mutex_handle;

#endif