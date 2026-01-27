/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "hts221.h"
#include "fifo.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define HAL_ARBITRARY_DELAY 0xFFFF

// Blocking I2C definitions
// Arbitrary number of trials
#define NUMBER_TRIALS 5
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
float celsius_to_farenheit(float degrees_celsius)
{
  return (9.0 / 5.0) * degrees_celsius + 32.0;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  // Wait until HTS221 is ready for communication
  if (HAL_I2C_IsDeviceReady(&hi2c2, HTS221_READ_ADDR, NUMBER_TRIALS, HAL_ARBITRARY_DELAY) != HAL_OK)
  {
    printf("HTS221 failed to setup.\r\n");
    Error_Handler();
  }
  printf("HTS221 is ready.\r\n");

  // Check device identity
  uint8_t identity = hts221_read_register_blocking(&hi2c2, WHO_AM_I);
  printf("HTS221 Identity: %x\r\n", identity);
  if (identity != 0xBC)
  {
    printf("HTS221 identity is invalid. Exiting.\r\n");
    Error_Handler();
  }

  // Write to registers
  hts221_write_register_blocking(&hi2c2, CTRL_REG1, 0x81);  // power on HTS221 and set output rate to 1 Hz
  hts221_write_register_blocking(&hi2c2, CTRL_REG2, 0x80);  // refresh content of internal registers

  // Initialize temperature calibration variables
  hts221_initialize_temperature_calibration(&hi2c2);

  // Temperature variables
  static int16_t prev_adc = 0;
  static int16_t curr_adc = 0;
  static float degrees_celsius;
  static float degrees_farenheit;

  // Initialize the FIFO that buffers temperature ADC data
  fifo_init();

  // Start reading the temperature via non-blocking interrupts
  HAL_I2C_Mem_Read_IT(&hi2c2, HTS221_READ_ADDR, TEMP_OUT_READ, REG_ADDR_SIZE, &fifo_buffer[fifo_write], DATA_SIZE2);

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if(fifo_read != fifo_write)
    {
      // Disable interrupts before reading FIFO
      HAL_NVIC_DisableIRQ(I2C2_EV_IRQn);

      // Read temperature from buffer
      // FIFO will be filled with alternating TEMP_OUT_L and TEMP_OUT_H bytes
      curr_adc = fifo_buffer[fifo_read];
      fifo_read = (fifo_read + DATA_SIZE1) % FIFO_SIZE;
      curr_adc |= (fifo_buffer[fifo_read] << CHAR_BIT);
      fifo_read = (fifo_read + DATA_SIZE1) % FIFO_SIZE;

      // Re-enable interrupts after reading FIFO
      HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);

      // Print temperature if the reading changed
      if(curr_adc != prev_adc)
      {
        // Convert ADC to celsius and farenheit values
        degrees_celsius = hts221_convert_temperature(curr_adc);
        degrees_farenheit = celsius_to_farenheit(degrees_celsius);

        // Use hex escaped symbols to print degree character
        printf("Temperature: %.2f \xf8\x43 / %.2f \xf8\x46\r\n", degrees_celsius, degrees_farenheit);
        prev_adc = curr_adc;
      }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  fifo_write = (fifo_write + DATA_SIZE2)  % FIFO_SIZE;
  HAL_I2C_Mem_Read_IT(&hi2c2, HTS221_READ_ADDR, TEMP_OUT_READ, REG_ADDR_SIZE, &fifo_buffer[fifo_write], DATA_SIZE2);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
