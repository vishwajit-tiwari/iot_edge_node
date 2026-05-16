#include "driver/i2c.h"
#include "esp_log.h"
#include "i2c_manager.h"

#define TAG "I2C_MANAGER"

#define I2C_PORT I2C_NUM_0
#define I2C_SDA 21
#define I2C_SCL 22

static bool is_initialized = false;

esp_err_t i2c_master_init_once(void)
{
    if (is_initialized)
    {
        ESP_LOGI(TAG, "I2C already initialized");
        return ESP_OK;
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &conf));

    esp_err_t ret = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);

    if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE)
    {
        is_initialized = true;
        ESP_LOGI(TAG, "I2C initialized successfully");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "I2C init failed");
    return ret;
}
