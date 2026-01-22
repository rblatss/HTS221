/*
 * HTS221 helper functions header 
 * Written by Robert Blatner
 * 
 */
#include "main.h"
#include <stdint.h>

// Definitions
#define T0_degC_x8 0x32
#define T1_degC_x8 0x33
#define T1_T0_msb 0x35
#define T0_OUT_LSB 0x3C
#define T0_OUT_MSB 0x3D
#define T1_OUT_LSB 0x3E
#define T1_OUT_MSB 0x3F
#define OUTPUT_TEMP_MASK ((uint16_t ) 0x3FF)

// Function declarations
uint8_t hts221_read_register_blocking(I2C_HandleTypeDef *hi2c, uint8_t register_address);
void hts221_write_register_blocking(I2C_HandleTypeDef *hi2c, uint8_t register_address, uint8_t data);
void initialize_temperature_calibration(I2C_HandleTypeDef *hi2c);
float linear_interpolation(uint16_t x, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1);
float convert_temperature(uint16_t t_out);
