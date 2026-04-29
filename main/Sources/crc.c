#include "crc.h"


uint8_t crc8_compute(const uint8_t *data, int len)
{
    uint8_t crc = 0x00;

    for (int i = 0; i < len; i++)
    {
        crc ^= data[i];

        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLY;
            else
                crc <<= 1;
        }
    }

    return crc;
}

uint16_t crc16_compute(const uint8_t *data, int len)
{
    uint16_t crc = 0xFFFF;

    for (int i = 0; i < len; i++)
    {
        crc ^= (data[i] << 8);

        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ CRC16_POLY;
            else
                crc <<= 1;
        }
    }

    return crc;
}

uint8_t checksum(const uint8_t *data, int len) 
{
    uint8_t checkSum = 0;

    for(int i=0; i<len; i++)
    {
        checkSum ^= data[i]; 
    }

    return checkSum;
}