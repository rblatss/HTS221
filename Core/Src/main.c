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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// FIFO definitions
#define FIFO_SIZE 32

// Non-blocking I2C definitions
#define DATA_SIZE 1

// Blocking I2C definitions
#define NUMBER_TRIALS 1

// HTS221 slave addresses
#define HTS221_READ_ADDR 0xBE
#define HTS221_WRITE_ADDR 0xBF

// HTS221 register addresses
#define WHO_AM_I 0x0F
#define AV_CONF 0x10
#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define STATUS_REG R 0x27
#define HUMIDITY_OUT_L 0x28
#define HUMIDITY_OUT_H 0x29
#define TEMP_OUT_L 0x2A
#define TEMP_OUT_H 0x2B

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

// Interrupt / main communication
bool check_for_temp_change = false;
uint8_t register_selection;

// FIFO variables
uint8_t fifo_buffer[FIFO_SIZE];
uint8_t fifo_read;
uint8_t fifo_write;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void fifo_init()
{
  memset(&fifo_buffer[0], 0, FIFO_SIZE);
  fifo_read = 0;
  fifo_write = 0;
}

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
  fifo_init();
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

  // Give HTS221 time to initialize
  HAL_Delay(1000);

  // Check that HTS221 is ready for communication 
  HAL_StatusTypeDef is_device_ready = HAL_I2C_IsDeviceReady(&hi2c2, HTS221_READ_ADDR, NUMBER_TRIALS, HAL_MAX_DELAY);
  if (is_device_ready != HAL_OK)
  {
    printf("Device is not ready. Exiting.\r\n");
    Error_Handler();
  }

  // Check device identity
  // Query WHO_AM_I register via blocking HAL transmit / receive calls
  uint8_t identity;
  register_selection = WHO_AM_I;
  HAL_I2C_Master_Transmit(&hi2c2, HTS221_WRITE_ADDR, &register_selection, NUMBER_TRIALS, HAL_MAX_DELAY);
  HAL_I2C_Master_Receive(&hi2c2, HTS221_READ_ADDR, &identity, NUMBER_TRIALS, HAL_MAX_DELAY);
  printf("HTS221 Identity: %x\r\n", identity);
  if (identity != 0xBC)
  {
    printf("HTS221 identity is invalid. Exiting.\r\n");
    Error_Handler();
  }

  // Start reading the temperature via non-blocking interrupts
  register_selection = TEMP_OUT_L;
  HAL_I2C_Master_Transmit_IT(&hi2c2, HTS221_WRITE_ADDR, &register_selection, DATA_SIZE);

  // Temperature variables
  int16_t prev_temp = 0;
  int16_t curr_temp = 0;
  float degrees_celsius;
  float degrees_farenheit;

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if(check_for_temp_change)
    {
      // Disable interrupts before printing
      HAL_NVIC_DisableIRQ(I2C2_EV_IRQn);

      // Read temperature from buffer
      // FIFO will be filled with alternating TEMP_OUT_L and TEMP_OUT_H bytes
      curr_temp = fifo_buffer[fifo_read];
      fifo_read = (fifo_read + DATA_SIZE) % FIFO_SIZE;
      curr_temp |= fifo_buffer[fifo_read] << 8;
      fifo_read = (fifo_read + DATA_SIZE) % FIFO_SIZE;

      // Print temperature if the reading changed
      if(curr_temp != prev_temp)
      {
        degrees_celsius = (curr_temp / 8.0);
        degrees_farenheit = celsius_to_farenheit(degrees_celsius);
        printf("Temp: %.2f \xf8\x43, %.2f \xf8\x46\r\n", degrees_celsius, degrees_farenheit);
        prev_temp = curr_temp;
      }

      // Wait for temperature data
      check_for_temp_change = false;

      // Re-enable interrupts after printing
      HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);
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

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00100D14;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if(hi2c == (I2C_HandleTypeDef *)&hi2c2)
  {
    // Alternate reading of TEMP_OUT_L and TEMP_OUT_H
    if(register_selection == TEMP_OUT_L)
    {
      register_selection = TEMP_OUT_H;
      HAL_I2C_Master_Transmit_IT(&hi2c2, HTS221_WRITE_ADDR, &register_selection, DATA_SIZE);
    }
    else if(register_selection == TEMP_OUT_H)
    {
      check_for_temp_change = true;  // check for temperature change after reading TEMP_OUT_H
      register_selection = TEMP_OUT_L;
      HAL_I2C_Master_Transmit_IT(&hi2c2, HTS221_WRITE_ADDR, &register_selection, DATA_SIZE);
    }
  }
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if(hi2c == (I2C_HandleTypeDef *)&hi2c2)
  {
    // Setup next receive interrupt
    HAL_I2C_Master_Receive_IT(&hi2c2, HTS221_READ_ADDR, &fifo_buffer[fifo_write], DATA_SIZE);

    // Increment write index, wrapping if needed
    fifo_write = (fifo_write + DATA_SIZE) % FIFO_SIZE;
  }
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
