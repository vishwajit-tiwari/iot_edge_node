/**
 * @file ina219_driver.c
 * @brief Driver implementation for INA219 current sensor.
 * 
 * This file contains the implementation of functions to initialize and read data from the INA219 sensor.
 * It uses I2C communication to interact with the sensor and provides functions to read voltage, current, and power values.
 * 
 * @author Vishwajit Kumar Tiwari
 * @date 11/04/2026
 */

#include "driver/i2c.h"
#include "esp_log.h"

#include "current_sensor_ina219.h"

#define TAG "INA219"

#define INA219_ADDR 0x40
#define I2C_PORT I2C_NUM_0

#define REG_CONFIG   0x00
#define REG_BUS_V    0x02
#define REG_CURRENT  0x04
#define REG_CALIB    0x05

static float current_lsb = 0.0f;

// ---------------- I2C helpers ----------------

static esp_err_t write_reg(uint8_t reg, uint16_t data)
{
    uint8_t buf[3] = {reg, data >> 8, data & 0xFF};
    return i2c_master_write_to_device(I2C_PORT, INA219_ADDR, buf, 3, pdMS_TO_TICKS(100));
}

static esp_err_t read_reg(uint8_t reg, uint16_t *data)
{
    uint8_t buf[2];
    esp_err_t err = i2c_master_write_read_device(
        I2C_PORT, INA219_ADDR, &reg, 1, buf, 2, pdMS_TO_TICKS(100));

    if (err != ESP_OK) return err;

    *data = (buf[0] << 8) | buf[1];
    return ESP_OK;
}

// ---------------- Calibration ----------------

static bool set_calibration(float max_current)
{
    float shunt = 0.1f;

    current_lsb = max_current / 32768.0f;

    uint16_t calib = (uint16_t)(0.04096f / (current_lsb * shunt));

    esp_err_t ret = write_reg(REG_CALIB, calib);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Calibration failed");
        return false;
    }
    else
    {
        ESP_LOGI(TAG, "Calibration successful");
        return true;
    }

    ESP_LOGI(TAG, "Calibration set: %.2fA", max_current);
}

// ---------------- Init ----------------

bool ina219_init(void)
{
    // Only sensor config here (I2C already initialized globally)

    if (write_reg(REG_CONFIG, 0x399F) != ESP_OK)
    {
        ESP_LOGE(TAG, "Config failed");
        return false;
    }

    const bool calibration_result = set_calibration(3.2f);
    if (!calibration_result)
    {
        ESP_LOGE(TAG, "INA219 calibration failed");
        return false;
    }

    ESP_LOGI(TAG, "INA219 initialized");

    return true;
}

// ---------------- Read ----------------

bool ina219_read(float *voltage, float *current, float *power)
{
    uint16_t raw;

    // Bus voltage
    if (read_reg(REG_BUS_V, &raw) != ESP_OK)
        return false;

    raw >>= 3;
    *voltage = raw * 0.004f;

    // Current
    if (read_reg(REG_CURRENT, &raw) != ESP_OK)
        return false;

    *current = (int16_t)raw * current_lsb;

    // Power
    *power = (*voltage) * (*current);

    return true;
}

bool ina219_recover(void)
{
    // Only sensor reconfiguration is required here since I2C driver is initialized globally and shared across tasks.
    // We just need to reset the sensor configuration and calibration to recover from a potential sensor fault.

    if(write_reg(REG_CONFIG, 0x399F) != ESP_OK)
    {
        return false;
    }

    const bool calibration_result = set_calibration(3.2f);
    if (!calibration_result)
    {
        ESP_LOGE(TAG, "INA219 recovery calibration failed\n");
        return false;
    }

    return calibration_result;
}