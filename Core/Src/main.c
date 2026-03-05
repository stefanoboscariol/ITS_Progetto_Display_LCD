/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE BEGIN 0 */
#include "string.h"

void LCD_Pulse_EN(void) {
    LL_GPIO_SetOutputPin(LCD_EN_GPIO_Port, LCD_EN_Pin);
    HAL_Delay(1);
    LL_GPIO_ResetOutputPin(LCD_EN_GPIO_Port, LCD_EN_Pin);
    HAL_Delay(1);
}

void LCD_Send_Nibble(uint8_t nibble) {
    // DB4..DB7
    if (nibble & 0x01) LL_GPIO_SetOutputPin(LCD_DB4_GPIO_Port, LCD_DB4_Pin);
    else               LL_GPIO_ResetOutputPin(LCD_DB4_GPIO_Port, LCD_DB4_Pin);

    if (nibble & 0x02) LL_GPIO_SetOutputPin(LCD_DB5_GPIO_Port, LCD_DB5_Pin);
    else               LL_GPIO_ResetOutputPin(LCD_DB5_GPIO_Port, LCD_DB5_Pin);

    if (nibble & 0x04) LL_GPIO_SetOutputPin(LCD_DB6_GPIO_Port, LCD_DB6_Pin);
    else               LL_GPIO_ResetOutputPin(LCD_DB6_GPIO_Port, LCD_DB6_Pin);

    if (nibble & 0x08) LL_GPIO_SetOutputPin(LCD_DB7_GPIO_Port, LCD_DB7_Pin);
    else               LL_GPIO_ResetOutputPin(LCD_DB7_GPIO_Port, LCD_DB7_Pin);

    LCD_Pulse_EN();
}

void LCD_Send_Byte(uint8_t byte, uint8_t isData) {
    // RW = 0 (write), RS = 0 cmd / 1 data
    LL_GPIO_ResetOutputPin(LCD_RW_GPIO_Port, LCD_RW_Pin);

    if (isData) LL_GPIO_SetOutputPin(LCD_RS_GPIO_Port, LCD_RS_Pin);
    else        LL_GPIO_ResetOutputPin(LCD_RS_GPIO_Port, LCD_RS_Pin);

    LCD_Send_Nibble(byte >> 4);   // nibble alto prima
    LCD_Send_Nibble(byte & 0x0F); // nibble basso
    HAL_Delay(2);
}

void LCD_Init(void) {
    HAL_Delay(50); // attesa power-on

    // Sequenza inizializzazione 4-bit (HD44780)
    LL_GPIO_ResetOutputPin(LCD_RS_GPIO_Port, LCD_RS_Pin);
    LL_GPIO_ResetOutputPin(LCD_RW_GPIO_Port, LCD_RW_Pin);

    LCD_Send_Nibble(0x03); HAL_Delay(5);
    LCD_Send_Nibble(0x03); HAL_Delay(1);
    LCD_Send_Nibble(0x03); HAL_Delay(1);
    LCD_Send_Nibble(0x02); HAL_Delay(1); // passa a 4-bit

    LCD_Send_Byte(0x28, 0); // 4-bit, 2 righe, 5x8
    LCD_Send_Byte(0x0C, 0); // display ON, cursore OFF
    LCD_Send_Byte(0x06, 0); // incremento automatico
    LCD_Send_Byte(0x01, 0); // clear display
    HAL_Delay(2);
}

void LCD_Print(const char *str) {
    while (*str) {
        LCD_Send_Byte((uint8_t)*str++, 1);
    }
}
void LCD_Set_Cursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? 0x00 : 0x40;
    address += col;
    LCD_Send_Byte(0x80 | address, 0);
}
/* USER CODE END 0 */
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
  /* USER CODE BEGIN 2 */
  /* USER CODE BEGIN 2 */
  /* USER CODE BEGIN 2 */
  LCD_Init();
  /*LCD_Set_Cursor(0, 0);
  LCD_Print("Stefano");
  LCD_Set_Cursor(1, 0);
  LCD_Print("Boscariol");*/
  /* USER CODE END 2 */
  /* USER CODE END 2 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint8_t i=0;
  char buff[16];
  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
	  /* USER CODE BEGIN 3 */
	  LCD_Send_Byte(0x01, 0);
	  HAL_Delay(2);
	  sprintf(buff,"Char %4d -> ",i);
	  LCD_Set_Cursor(0, 0);
	  LCD_Print(buff);
	  LCD_Send_Byte(i, 1);
	  sprintf(buff,"Char 0x%02X -> %c",i,i);
	  LCD_Set_Cursor(1, 0);
	  LCD_Print(buff);
	  i+=1;
	  HAL_Delay(10);
	  /* USER CODE END 3 */
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

  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

  /**/
  LL_GPIO_ResetOutputPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin);

  /**/
  LL_GPIO_ResetOutputPin(INTERNAL_LED_GPIO_Port, INTERNAL_LED_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LCD_DB4_GPIO_Port, LCD_DB4_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LCD_EN_GPIO_Port, LCD_EN_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LCD_DB7_GPIO_Port, LCD_DB7_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LCD_DB5_GPIO_Port, LCD_DB5_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LCD_DB6_GPIO_Port, LCD_DB6_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LCD_RW_GPIO_Port, LCD_RW_Pin);

  /**/
  LL_GPIO_ResetOutputPin(LCD_RS_GPIO_Port, LCD_RS_Pin);

  /**/
  GPIO_InitStruct.Pin = USER_BUTTON_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(USER_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = VCP_USART2_TX_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(VCP_USART2_TX_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = VCP_USART2_RX_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(VCP_USART2_RX_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = INTERNAL_LED_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(INTERNAL_LED_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_DB4_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_DB4_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_EN_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_EN_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_DB7_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_DB7_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_DB5_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_DB5_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_DB6_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_DB6_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_RW_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_RW_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_RS_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_RS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
