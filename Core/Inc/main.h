/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32c0xx_hal.h"
#include "stm32c0xx_ll_system.h"
#include "stm32c0xx_ll_gpio.h"
#include "stm32c0xx_ll_exti.h"
#include "stm32c0xx_ll_bus.h"
#include "stm32c0xx_ll_cortex.h"
#include "stm32c0xx_ll_rcc.h"
#include "stm32c0xx_ll_utils.h"
#include "stm32c0xx_ll_pwr.h"
#include "stm32c0xx_ll_dma.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USER_BUTTON_Pin LL_GPIO_PIN_13
#define USER_BUTTON_GPIO_Port GPIOC
#define RCC_OSCX_IN_Pin LL_GPIO_PIN_14
#define RCC_OSCX_IN_GPIO_Port GPIOC
#define RCC_OSCX_OUT_Pin LL_GPIO_PIN_15
#define RCC_OSCX_OUT_GPIO_Port GPIOC
#define RCC_OSC_IN_Pin LL_GPIO_PIN_0
#define RCC_OSC_IN_GPIO_Port GPIOF
#define RCC_OSC_OUT_Pin LL_GPIO_PIN_1
#define RCC_OSC_OUT_GPIO_Port GPIOF
#define VCP_USART2_TX_Pin LL_GPIO_PIN_2
#define VCP_USART2_TX_GPIO_Port GPIOA
#define VCP_USART2_RX_Pin LL_GPIO_PIN_3
#define VCP_USART2_RX_GPIO_Port GPIOA
#define INTERNAL_LED_Pin LL_GPIO_PIN_5
#define INTERNAL_LED_GPIO_Port GPIOA
#define LCD_DB4_Pin LL_GPIO_PIN_10
#define LCD_DB4_GPIO_Port GPIOB
#define LCD_EN_Pin LL_GPIO_PIN_10
#define LCD_EN_GPIO_Port GPIOA
#define DEBUG_SWDIO_Pin LL_GPIO_PIN_13
#define DEBUG_SWDIO_GPIO_Port GPIOA
#define DEBUG_SWCLK_Pin LL_GPIO_PIN_14
#define DEBUG_SWCLK_GPIO_Port GPIOA
#define LCD_DB7_Pin LL_GPIO_PIN_15
#define LCD_DB7_GPIO_Port GPIOA
#define LCD_DB5_Pin LL_GPIO_PIN_4
#define LCD_DB5_GPIO_Port GPIOB
#define LCD_DB6_Pin LL_GPIO_PIN_5
#define LCD_DB6_GPIO_Port GPIOB
#define LCD_RW_Pin LL_GPIO_PIN_6
#define LCD_RW_GPIO_Port GPIOB
#define LCD_RS_Pin LL_GPIO_PIN_7
#define LCD_RS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
