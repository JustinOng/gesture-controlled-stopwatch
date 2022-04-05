#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

constexpr uint8_t SENSOR_IMAGE_WIDTH = 8;
constexpr uint8_t SENSOR_IMAGE_SIZE = SENSOR_IMAGE_WIDTH * SENSOR_IMAGE_WIDTH;

// VL53L5CX-SATEL has a power enable pin that must be driven HIGH
constexpr int PIN_POWER = 2;

int8_t sensor_setup();
int8_t sensor_read(float distances[], uint16_t sigma[]);

#endif
