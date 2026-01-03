/*
 * stdio_handler.c
 *
 *  Created on: Aug 27, 2023
 *      Author: Glenn Andrews
 *  Copied from: https://forum.digikey.com/t/easily-use-printf-on-stm32/20157
 */

#include <stdio.h>
#include "stm32l4xx_hal.h"

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

extern UART_HandleTypeDef huart1;

PUTCHAR_PROTOTYPE
{
  HAL_StatusTypeDef status =
  		HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return (status == 0) ? ch : EOF;
}
