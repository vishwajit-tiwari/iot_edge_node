/**
 * @file current_sensor_ina219.h
 * @brief Header file for INA219 current sensor module.
 * 
 * This module provides initialization and reading functions for the INA219 current sensor.
 * It allows you to read voltage, current, and power values from the sensor.
 * 
 * @author Vishwajit Kumar Tiwari
 * @date 11/04/2026
 */

#ifndef CURRENT_SENSOR_INA219_H
#define CURRENT_SENSOR_INA219_H

#include <stdbool.h>

bool ina219_init(void);
bool ina219_read(float *voltage, float *current, float *power);

#endif // CURRENT_SENSOR_INA219_H
