#ifndef CURRENT_SENSOR_INA219_H
#define CURRENT_SENSOR_INA219_H

#include <stdbool.h>

bool ina219_init(void);
bool ina219_read(float *voltage, float *current, float *power);

#endif