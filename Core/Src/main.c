/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>   // aggiunta per seriale
#include <stdio.h>
#include "string.h"
#include <ctype.h>
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

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// Variabile semaforo: 1 = il display si aggiorna, 0 = in pausa
volatile uint8_t lcd_running = 1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



void LCD_Pulse_EN(void) {
    // 1. Piccolissimo ritardo per far stabilizzare i dati prima di dare l'Enable (Setup Time)
    for(volatile int x=0; x<5; x++);

    // 2. Alza Enable
    LL_GPIO_SetOutputPin(LCD_EN_GPIO_Port, LCD_EN_Pin);
    for(volatile int x=0; x<20; x++); // Ritardo per fargli leggere il dato

    // 3. Abbassa Enable
    LL_GPIO_ResetOutputPin(LCD_EN_GPIO_Port, LCD_EN_Pin);
    for(volatile int x=0; x<20; x++); // Ritardo di Hold Time
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

// FUNZIONE CORRETTA: Legge il Busy Flag evitando cortocircuiti logici
void LCD_WaitBusy(void) {
    uint8_t isBusy = 1;
    uint32_t timeout = 50000;

    // FONDAMENTALE: Imposta TUTTI i pin dati come INPUT prima di leggere!
    LL_GPIO_SetPinMode(LCD_DB4_GPIO_Port, LCD_DB4_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(LCD_DB5_GPIO_Port, LCD_DB5_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(LCD_DB6_GPIO_Port, LCD_DB6_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(LCD_DB7_GPIO_Port, LCD_DB7_Pin, LL_GPIO_MODE_INPUT);

    // RS = 0 (Comando), RW = 1 (Lettura)
    LL_GPIO_ResetOutputPin(LCD_RS_GPIO_Port, LCD_RS_Pin);
    LL_GPIO_SetOutputPin(LCD_RW_GPIO_Port, LCD_RW_Pin);

    while (isBusy && timeout > 0) {
        // Leggi il primo nibble (dove si trova il Busy Flag su DB7)
        LL_GPIO_SetOutputPin(LCD_EN_GPIO_Port, LCD_EN_Pin);
        for(volatile int x=0; x<15; x++);
        isBusy = LL_GPIO_IsInputPinSet(LCD_DB7_GPIO_Port, LCD_DB7_Pin);
        LL_GPIO_ResetOutputPin(LCD_EN_GPIO_Port, LCD_EN_Pin);
        for(volatile int x=0; x<15; x++);

        // Leggi il secondo nibble (Obbligatorio a 4 bit per chiudere il ciclo!)
        LL_GPIO_SetOutputPin(LCD_EN_GPIO_Port, LCD_EN_Pin);
        for(volatile int x=0; x<15; x++);
        LL_GPIO_ResetOutputPin(LCD_EN_GPIO_Port, LCD_EN_Pin);
        for(volatile int x=0; x<15; x++);

        timeout--;
    }

    // Ripristina la scrittura (RW=0) e rimetti TUTTI i pin come OUTPUT
    LL_GPIO_ResetOutputPin(LCD_RW_GPIO_Port, LCD_RW_Pin);
    LL_GPIO_SetPinMode(LCD_DB4_GPIO_Port, LCD_DB4_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(LCD_DB5_GPIO_Port, LCD_DB5_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(LCD_DB6_GPIO_Port, LCD_DB6_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(LCD_DB7_GPIO_Port, LCD_DB7_Pin, LL_GPIO_MODE_OUTPUT);
}

void LCD_Send_Byte(uint8_t byte, uint8_t isData) {
    // 1. Aspetta la conferma dal display usando il Flag (velocissimo!)
    LCD_WaitBusy();

    // 2. Invia i dati
    LL_GPIO_ResetOutputPin(LCD_RW_GPIO_Port, LCD_RW_Pin);

    if (isData) LL_GPIO_SetOutputPin(LCD_RS_GPIO_Port, LCD_RS_Pin);
    else        LL_GPIO_ResetOutputPin(LCD_RS_GPIO_Port, LCD_RS_Pin);

    LCD_Send_Nibble(byte >> 4);   // nibble alto
    LCD_Send_Nibble(byte & 0x0F); // nibble basso
}

void LCD_Init(void) {
    HAL_Delay(50); // attesa power-on

    // Inizializzazione 4-bit forzata a mano (senza leggere il flag)
    LL_GPIO_ResetOutputPin(LCD_RS_GPIO_Port, LCD_RS_Pin);
    LL_GPIO_ResetOutputPin(LCD_RW_GPIO_Port, LCD_RW_Pin);

    LCD_Send_Nibble(0x03); HAL_Delay(5);
    LCD_Send_Nibble(0x03); HAL_Delay(1);
    LCD_Send_Nibble(0x03); HAL_Delay(1);
    LCD_Send_Nibble(0x02); HAL_Delay(1); // passa a 4-bit

    // Da qui in poi Send_Byte userà il Busy Flag per viaggiare al massimo!
    LCD_Send_Byte(0x28, 0); // 4-bit, 2 righe, 5x8
    LCD_Send_Byte(0x0C, 0); // display ON, cursore OFF
    LCD_Send_Byte(0x06, 0); // incremento automatico
    LCD_Send_Byte(0x01, 0); // clear display
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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();

  // Variabili richieste prima del ciclo while
  uint8_t i = 0;
  char buff[16];
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // Se il semaforo è verde (1), aggiorna il display
    if (lcd_running == 1)
    {
        LCD_Send_Byte(0x01, 0); // lcdSendCmd(0x01) -> Pulisce il display
        // RIMOSSO HAL_Delay(2); perché c'è il Busy Flag!

        // Stampa sulla prima riga
        sprintf(buff, "Char %4d -> ", i);
        LCD_Set_Cursor(0, 0);
        LCD_Print(buff);
        LCD_Send_Byte(i, 1);    // lcdSendChar(i) -> Invia il singolo carattere ASCII

        // Stampa sulla seconda riga (in Esadecimale)
        sprintf(buff, "Char 0x%02X -> %c", i, i);
        LCD_Set_Cursor(1, 0);
        LCD_Print(buff);

        i += 1; // Passa al carattere successivo
        char uart_msg[40];
        		sprintf(uart_msg, "Char %4d -> %c\r\nChar 0x%02X -> %c\r\n\r\n", i, isprint(i) ? i : '.', i, isprint(i) ? i : '.');
                HAL_UART_Transmit(&huart2, (uint8_t*)uart_msg, strlen(uart_msg), HAL_MAX_DELAY);
    }

    // Aspetta in ogni caso 750ms prima di ripetere il ciclo
    HAL_Delay(750);

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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
    NVIC_SetPriority(EXTI4_15_IRQn, 0); // Imposta la priorità (0 = massima)
    NVIC_EnableIRQ(EXTI4_15_IRQn);      // Abilita l'interrupt
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief Questa funzione gestisce gli interrupt delle linee EXTI da 4 a 15.
  */
void EXTI4_15_IRQHandler(void)
{
  // Verifica se l'interrupt è stato generato dalla linea 13 sul fronte di salita (RISING)
  if (LL_EXTI_IsActiveRisingFlag_0_31(LL_EXTI_LINE_13) != RESET)
  {
    // 1. Pulisce la flag dell'interrupt RISING (FONDAMENTALE)
    LL_EXTI_ClearRisingFlag_0_31(LL_EXTI_LINE_13);

    // 2. Controllo anti-rimbalzo (Debouncing)
    // Memorizza l'ultimo momento in cui hai premuto il tasto
    static uint32_t ultimo_istante_pressione = 0;
    uint32_t istante_attuale = HAL_GetTick(); // Prende il tempo in millisecondi

    // Se sono passati almeno 200 millisecondi dall'ultima pressione vera...
    if ((istante_attuale - ultimo_istante_pressione) > 200)
    {
        // 3. Inverti lo stato di esecuzione (se era 1 diventa 0, se 0 diventa 1)
        lcd_running = !lcd_running;

        // 4. Inverti il LED per avere un feedback visivo immediato
        LL_GPIO_TogglePin(INTERNAL_LED_GPIO_Port, INTERNAL_LED_Pin);

        // Salva il tempo di questa pressione
        ultimo_istante_pressione = istante_attuale;
    }
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
