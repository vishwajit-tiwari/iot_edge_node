#include "driver/i2c.h"
#include "esp_log.h"

#include "current_sensor_ina219.h"

#define INA219_ADDR 0x40

#define I2C_SDA 21
#define I2C_SCL 22

#define I2C_PORT I2C_NUM_0

static const char *TAG = "INA219";

static esp_err_t ina219_i2c_write(uint8_t reg, uint16_t data)
{
    uint8_t write_buffer[3];  // Register address + 2 bytes of data
    
    write_buffer[0] = reg;  // Register address
    write_buffer[1] = (data >> 8); // High byte
    write_buffer[2] = data & 0xFF; // Low byte

    // Write the register address followed by the data
    esp_err_t status = i2c_master_write_to_device(I2C_PORT, INA219_ADDR, write_buffer, sizeof(write_buffer), pdMS_TO_TICKS(1000));
    if(status != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write to INA219: %s", esp_err_to_name(status));
    }

    return status;
}

static esp_err_t ina219_i2c_read(uint8_t reg, uint16_t *data)
{
    uint8_t read_buff[2];  // Buffer to hold the 2 bytes of data read from the device

    // First, write the register address we want to read from, then read the data
    esp_err_t status = i2c_master_write_read_device(I2C_PORT, INA219_ADDR, &reg, sizeof(reg), read_buff, sizeof(read_buff), pdMS_TO_TICKS(1000));
    if(status != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read from INA219: %s", esp_err_to_name(status));
    }

    // Combine the two bytes into a single 16-bit value
    *data = (read_buff[0] << 8) | read_buff[1]; // Combine high and low bytes

    return status;
}

bool ina219_init(void)
{
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000, // 100 kHz or 1MHz
    };

    if(i2c_param_config(I2C_PORT, &i2c_config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure I2C parameters");
        return false;
    }

    if(i2c_driver_install(I2C_PORT, i2c_config.mode, 0, 0, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to install I2C driver");
        return false;
    }

    if(ina219_i2c_write(0x00, 0x399F) != ESP_OK) // Configuration register: 16V range, 320mA, 12-bit ADC
    {
        ESP_LOGE(TAG, "Failed to configure INA219");
        return false;
    }

    return true;
}

bool ina219_read(float *voltage, float *current, float *power)
{
    uint16_t raw_value;

    if(ina219_i2c_read(0x02, &raw_value) != ESP_OK) // Shunt voltage register
    {
        ESP_LOGE(TAG, "Failed to read shunt voltage");
        return false;
    }

    *voltage = raw_value * 0.004; // Convert to volts (4mV per bit)

    if(ina219_i2c_read(0x04, &raw_value) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read current");
        return false;
    }

    *current = raw_value * 0.01; // Convert to current (10A per bit)

    *power = (*voltage) * (*current); 

    return true;
}