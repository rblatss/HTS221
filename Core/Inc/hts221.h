/*
 * HTS221 helper functions header 
 * Written by Robert Blatner
 * 
 */
#ifndef HTS221
#define HTS221

#include "main.h"
#include <stdint.h>

// HTS221 device addresses
#define HTS221_WRITE_ADDR 0xBE
#define HTS221_READ_ADDR 0xBF

// HTS221 register addresses
#define WHO_AM_I 0x0F
#define AV_CONF 0x10
#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define STATUS_REG 0x27
#define HUMIDITY_OUT_L 0x28
#define HUMIDITY_OUT_H 0x29
#define TEMP_OUT_L 0x2A
#define TEMP_OUT_H 0x2B

// Temperature calibration register addresses
#define T0_degC_x8 0x32
#define T1_degC_x8 0x33
#define T1_T0_msb 0x35
#define T0_OUT_LSB 0x3C
#define T0_OUT_MSB 0x3D
#define T1_OUT_LSB 0x3E
#define T1_OUT_MSB 0x3F

// Special bit for multi-byte reception
#define HTS221_MULTI_BYTE_FLAG 0x80
#define TEMP_OUT_READ (TEMP_OUT_L | HTS221_MULTI_BYTE_FLAG)


// Function declarations
uint8_t hts221_read_register_blocking(I2C_HandleTypeDef *hi2c, uint8_t register_address);

void hts221_write_register_blocking(I2C_HandleTypeDef *hi2c, uint8_t register_address, uint8_t data);

void hts221_initialize_temperature_calibration(I2C_HandleTypeDef *hi2c);

float linear_interpolation(uint16_t x, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1);

float hts221_convert_temperature(uint16_t t_out);

#endif
