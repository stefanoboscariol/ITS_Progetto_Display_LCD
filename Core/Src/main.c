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
#include <stdio.h>
#include "string.h"
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
// Variabile semaforo: 1 = il display si aggiorna, 0 = in pausa
volatile uint8_t lcd_running = 1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
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
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
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
    }

    // Aspetta in ogni caso 750ms prima di ripetere il ciclo
    HAL_Delay(750);

  }
  /* USER CODE END 3 */
}

/*
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
  LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

  /**/
  LL_GPIO_ResetOutputPin(INTERNAL_LED_GPIO_Port, INTERNAL_LED_Pin);
  LL_GPIO_ResetOutputPin(LCD_DB4_GPIO_Port, LCD_DB4_Pin);
  LL_GPIO_ResetOutputPin(LCD_EN_GPIO_Port, LCD_EN_Pin);
  LL_GPIO_ResetOutputPin(LCD_DB7_GPIO_Port, LCD_DB7_Pin);
  LL_GPIO_ResetOutputPin(LCD_DB5_GPIO_Port, LCD_DB5_Pin);
  LL_GPIO_ResetOutputPin(LCD_DB6_GPIO_Port, LCD_DB6_Pin);
  LL_GPIO_ResetOutputPin(LCD_RW_GPIO_Port, LCD_RW_Pin);
  LL_GPIO_ResetOutputPin(LCD_RS_GPIO_Port, LCD_RS_Pin);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_13;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /**/
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_13, LL_GPIO_PULL_NO);
  LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTC, LL_EXTI_CONFIG_LINE13);

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
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_DB4_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_EN_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_EN_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_DB7_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_DB7_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_DB5_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_DB5_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_DB6_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_DB6_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_RW_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_RW_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LCD_RS_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
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
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
