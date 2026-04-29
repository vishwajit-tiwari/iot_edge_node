#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#define CRC8_POLY 0x07
#define CRC16_POLY 0x1021

uint8_t crc8_compute(const uint8_t *data, int len);
uint16_t crc16_compute(const uint8_t *data, int len);
uint8_t checksum(const uint8_t *data, int len);

#endif