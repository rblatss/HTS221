/*
 * HTS221 helper functions implementation 
 * Written by Robert Blatner
 * 
 */
#include "hts221.h"

// Temperature globals
static uint16_t t0_out;
static uint16_t t1_out;
static uint16_t t0_degc;
static uint16_t t1_degc;

uint8_t hts221_read_register_blocking(I2C_HandleTypeDef *hi2c, uint8_t register_address)
{
  HAL_StatusTypeDef result;
  uint8_t data;
  result = HAL_I2C_Mem_Read(hi2c, HTS221_READ_ADDR, register_address, REG_ADDR_SIZE, &data, DATA_SIZE1, HAL_MAX_DELAY);
  if(result != HAL_OK)
  {
    printf("Error reading register %x\r\n", register_address);
  }
  return data;
}

void hts221_write_register_blocking(I2C_HandleTypeDef *hi2c, uint8_t register_address, uint8_t data)
{
  HAL_StatusTypeDef result;
  result = HAL_I2C_Mem_Write(hi2c, HTS221_WRITE_ADDR, register_address, REG_ADDR_SIZE, &data, DATA_SIZE1, HAL_MAX_DELAY);
  if(result != HAL_OK)
  {
    printf("Error writing to register %x\r\n", register_address);
  }
}

void hts221_initialize_temperature_calibration(I2C_HandleTypeDef *hi2c)
{
  // Read calibration values
  uint8_t t0_out_lsb, t0_out_msb, t1_out_lsb, t1_out_msb;
  uint8_t t0_degc_x8, t1_degc_x8, t1_t0_msb;

  // ADC values (x-axis)
  t0_out_lsb = hts221_read_register_blocking(hi2c, T0_OUT_LSB);
  t0_out_msb = hts221_read_register_blocking(hi2c, T0_OUT_MSB);
  t1_out_lsb = hts221_read_register_blocking(hi2c, T1_OUT_LSB);
  t1_out_msb = hts221_read_register_blocking(hi2c, T1_OUT_MSB);
  t0_out = (((uint16_t) t0_out_msb) << CHAR_BIT) | ((uint16_t) t0_out_lsb);
  t1_out = (((uint16_t) t1_out_msb) << CHAR_BIT) | ((uint16_t) t1_out_lsb);

  // Temperature values (y-axis)
  t0_degc_x8 = hts221_read_register_blocking(hi2c, T0_degC_x8);
  t1_degc_x8 = hts221_read_register_blocking(hi2c, T1_degC_x8);
  t1_t0_msb = hts221_read_register_blocking(hi2c, T1_T0_msb);
  t0_degc = (((uint16_t) (0x3 & t1_t0_msb) << CHAR_BIT) | (uint16_t) t0_degc_x8);
  t1_degc = (((uint16_t) (0xC & t1_t0_msb) << (CHAR_BIT - 2)) | (uint16_t) t1_degc_x8);
}

float linear_interpolation(uint16_t x, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1)
{
  return y0 + (x - x0) * (((float)(y1 - y0)) / (x1 - x0));
}

float hts221_convert_temperature(uint16_t t_out)
{
  return linear_interpolation(t_out, t0_out, t1_out, t0_degc, t1_degc) / 8.0;
}
