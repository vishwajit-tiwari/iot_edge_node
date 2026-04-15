#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "hal/gpio_types.h"

#include "temp_sensor_ds18b20.h"
#include "sensor.h"

#define DS18B20_GPIO GPIO_NUM_4

/* ================= LOW LEVEL 1-WIRE ================= */
static void ds18b20_set_output()
{
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
}

static void ds18b20_set_input()
{
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
}

static void ds18b20_write_low()
{
    gpio_set_level(DS18B20_GPIO, 0);
}

static int ds18b20_read_pin()
{
    return gpio_get_level(DS18B20_GPIO);
}

/* ================= RESET ================= */
static int ds18b20_reset()
{
    ds18b20_set_output();
    ds18b20_write_low();

    esp_rom_delay_us(480);

    ds18b20_set_input();

    esp_rom_delay_us(80);

    int presence = !ds18b20_read_pin();

    esp_rom_delay_us(400);

    return presence;
}

/* ================= WRITE BIT ================= */
static void ds18b20_write_bit(int bit)
{
    ds18b20_set_output();
    ds18b20_write_low();

    if(bit)
    {
        esp_rom_delay_us(5);
        ds18b20_set_input();
        esp_rom_delay_us(55);
    }
    else
    {
        esp_rom_delay_us(60);
        ds18b20_set_input();
    }
}

/* ================= READ BIT ================= */
static int ds18b20_read_bit()
{
    int bit = 0;

    ds18b20_set_output();
    ds18b20_write_low();

    esp_rom_delay_us(3);

    ds18b20_set_input();

    esp_rom_delay_us(10);

    bit = ds18b20_read_pin();

    esp_rom_delay_us(50);

    return bit;
}

/* ================= WRITE BYTE ================= */
static void ds18b20_write_byte(uint8_t byte)
{
    for(int i=0; i<8; i++)
    {
        int bit = (byte & 0x01);
        ds18b20_write_bit(bit);
        byte = byte >>1;
    }
}

/* ================= READ BYTE ================= */
static uint8_t ds18b20_read_byte()
{
    uint8_t byte = 0;

    for(int i=0; i<8; i++)
    {
        int bit = ds18b20_read_bit();
        byte = byte | (bit << i);
    }

    return byte;
}

/* ================= PUBLIC API ================= */
void temp_sensor_ds18b20_init()
{
    gpio_set_pull_mode(DS18B20_GPIO, GPIO_PULLUP_ONLY);
}

/* Start conversion (non-blocking) */
void temp_sensor_ds18b20_start()
{
    if(!ds18b20_reset())
    {
        return;
    }

    ds18b20_write_byte(0xCC);  // skip rom
    ds18b20_write_byte(0x44);  // convert T
}

/* Read temperature */
float temp_sensor_ds18b20_read()
{
    uint8_t temp_l, temp_h;

    if(!ds18b20_reset())
    {
        return -1000; 
    } 

    ds18b20_write_byte(0xCC);
    ds18b20_write_byte(0xBE);

    temp_l = ds18b20_read_byte();
    temp_h = ds18b20_read_byte();

    int16_t temp = (temp_h << 8) | temp_l;

    return temp/16.0;
}
