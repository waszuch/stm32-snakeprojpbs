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
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "nokia5110_LCD.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint8_t x;
    uint8_t y;
} Point;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define USART_TXBUF_LEN    1512
#define USART_RXBUF_LEN    512
#define FRAME_START        '['
#define FRAME_END          ']'
#define FRAME_MAX_LENGTH   500
#define CRC_INIT           0xFFFF


#define SNAKE_MAX_LENGTH 50
#define GRID_WIDTH  84
#define GRID_HEIGHT 48
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t USART_TxBuf[USART_TXBUF_LEN];
uint8_t USART_RxBuf[USART_RXBUF_LEN];

__IO int USART_TX_Empty = 0;
__IO int USART_TX_Busy = 0;
__IO int USART_RX_Empty = 0;
__IO int USART_RX_Busy = 0;




uint16_t joystick[2];



Point snake[SNAKE_MAX_LENGTH];
uint8_t snake_length = 3;


Point food;


Point direction = {1, 0};

bool game_over = false;

volatile int16_t snakeTimer = 0;
volatile int gameTimerState = 1;

typedef enum {
    WAITING_FOR_FRAME,
    COLLECTING_FRAME,
    ESCAPING_CHARACTER
} FrameState;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
uint8_t USART_kbhit(void);

int8_t USART_getchar(void);

void USART_fsend(char* format, ...);

uint16_t calculate_crc(const uint8_t *data, size_t length);

uint16_t ascii_to_uint16(const uint8_t *ascii_crc);

void uint16_to_ascii_hex(uint16_t value, char *output);

uint16_t computeFrameCRC(const char *sender, const char *receiver,
                         const char *command, const char *data);

void processCommand(const char *command, char *data);


void sendResponse(const char *sender, const char *receiver,
                  const char *command, const char *data);

void handleFrame(const uint8_t *frame, size_t length);

uint8_t* getFrame(size_t *outLength);


void sendErrorResponse(const char *receiver, const char *sender, const char *errorCode);


void generate_food(void);
void handle_joystick_input(void);
void move_snake(void);
bool check_collision(Point head);
void update_display(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  LCD_setRST(RST_GPIO_Port, RST_Pin);
      LCD_setCE(CE_GPIO_Port,   CE_Pin);
      LCD_setDC(DC_GPIO_Port,   DC_Pin);
      LCD_setDIN(GPIOA,         Din_Pin);
      LCD_setCLK(GPIOA,         Clk_Pin);
     LCD_init();
      LCD_clrScr();


      snake_length = 3;
      game_over    = false;
      direction.x  = 1;
      direction.y  = 0;



      snake[0].x = 40; snake[0].y = 24;
      snake[1].x = 39; snake[1].y = 24;
      snake[2].x = 38; snake[2].y = 24;

      // Jedzenie
      generate_food();

  HAL_UART_Receive_IT(&huart2, &USART_RxBuf[USART_RX_Empty], 1);

  HAL_TIM_Base_Start(&htim3);

  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)joystick, 2);

  snakeTimer = 10;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	     if (snakeTimer == 0)
	     {

	       if (gameTimerState == 1 && !game_over)
	       {
	         handle_joystick_input();
	         move_snake();
	         update_display();
	       }
	       else if (game_over)
	       {

	         LCD_clrScr();

	         LCD_refreshScr();


	         game_over    = false;
	         snake_length = 3;
	         direction.x  = 1;
	         direction.y  = 0;
	         snake[0].x   = 40; snake[0].y = 24;
	         snake[1].x   = 39; snake[1].y = 24;
	         snake[2].x   = 38; snake[2].y = 24;
	         generate_food();
	       }

	       // Niezależnie czy wąż rusza się, czy pauza – ustawiamy licznik znowu na 10
	       snakeTimer = 10;
	     }
	  size_t length = 0;
	      uint8_t *frame = getFrame(&length);
	      if (frame)
	      {

	          handleFrame(frame, length);
	      }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 49;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 7999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

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
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, Clk_Pin|Din_Pin|DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CE_GPIO_Port, CE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : Clk_Pin Din_Pin DC_Pin */
  GPIO_InitStruct.Pin = Clk_Pin|Din_Pin|DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : RST_Pin */
  GPIO_InitStruct.Pin = RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PC10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : CE_Pin */
  GPIO_InitStruct.Pin = CE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CE_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void generate_food(void)
{
  bool valid;
  do {
    valid = true;
    food.x = rand() % GRID_WIDTH;
    food.y = rand() % GRID_HEIGHT;
    for (uint8_t i = 0; i < snake_length; i++)
    {
      if (snake[i].x == food.x && snake[i].y == food.y)
      {
        valid = false;
        break;
      }
    }
  } while (!valid);
}

void move_snake(void)
{
  Point new_head = { snake[0].x + direction.x, snake[0].y + direction.y };
  if (check_collision(new_head))
  {
    game_over = true;
    return;
  }
  if (new_head.x == food.x && new_head.y == food.y)
  {
    snake_length++;
    generate_food();
  }
  for (int i = snake_length - 1; i > 0; i--)
  {
    snake[i] = snake[i - 1];
  }
  snake[0] = new_head;
}

bool check_collision(Point head)
{
  if (head.x >= GRID_WIDTH || head.y >= GRID_HEIGHT)
    return true;
  for (uint8_t i = 0; i < snake_length; i++)
  {
    if (head.x == snake[i].x && head.y == snake[i].y)
      return true;
  }
  return false;
}

void update_display(void)
{
  LCD_clrScr();
  LCD_setPixel(food.x, food.y, true);
  for (uint8_t i = 0; i < snake_length; i++)
  {
    LCD_setPixel(snake[i].x, snake[i].y, true);
  }
  LCD_refreshScr();
}

void handle_joystick_input(void)
{


  if (joystick[0] < 1500 && direction.y == 0)
  {
    // "dół"
    direction.x = 0;
    direction.y = 1;
  }
  else if (joystick[0] > 2500 && direction.y == 0)
  {
    // "góra"
    direction.x = 0;
    direction.y = -1;
  }

  else if (joystick[1] < 1500 && direction.x == 0)
  {
    direction.x = -1;
    direction.y = 0;
  }
  else if (joystick[1] > 2500 && direction.x == 0)
  {
    direction.x = 1;
    direction.y = 0;
  }
}

uint8_t USART_kbhit(void)
{
  if (USART_RX_Empty == USART_RX_Busy) {
    return 0;
  } else {
    return 1;
  }
}

int8_t USART_getchar(void)
{
  if (USART_RX_Empty != USART_RX_Busy) {
    int8_t tmp = USART_RxBuf[USART_RX_Busy];
    USART_RX_Busy++;
    if (USART_RX_Busy >= USART_RXBUF_LEN) {
      USART_RX_Busy = 0;
    }
    return tmp;
  }
  return -1;
}

void USART_fsend(char* format, ...)
{
  char tmp_rs[220];
  int i;
  __IO int idx;
  va_list arglist;

  va_start(arglist, format);
  vsprintf(tmp_rs, format, arglist);
  va_end(arglist);

  idx = USART_TX_Empty;
  for (i = 0; i < (int)strlen(tmp_rs); i++) {
    USART_TxBuf[idx] = (uint8_t)tmp_rs[i];
    idx++;
    if (idx >= USART_TXBUF_LEN) idx = 0;
  }

  __disable_irq();
  if ((USART_TX_Empty == USART_TX_Busy) &&
      (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TXE) == SET)) {
    USART_TX_Empty = idx;
    uint8_t tmp = USART_TxBuf[USART_TX_Busy];
    USART_TX_Busy++;
    if (USART_TX_Busy >= USART_TXBUF_LEN) USART_TX_Busy = 0;
    HAL_UART_Transmit_IT(&huart2, &tmp, 1);
  } else {
    USART_TX_Empty = idx;
  }
  __enable_irq();
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart2) {
    if (USART_TX_Empty != USART_TX_Busy) {
      uint8_t tmp = USART_TxBuf[USART_TX_Busy];
      USART_TX_Busy++;
      if (USART_TX_Busy >= USART_TXBUF_LEN) USART_TX_Busy = 0;
      HAL_UART_Transmit_IT(&huart2, &tmp, 1);
    }
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart2) {
    USART_RX_Empty++;
    if (USART_RX_Empty >= USART_RXBUF_LEN) USART_RX_Empty = 0;
    HAL_UART_Receive_IT(&huart2, &USART_RxBuf[USART_RX_Empty], 1);
  }
}


uint16_t calculate_crc(const uint8_t *data, size_t length)
{
  uint16_t crc = CRC_INIT;
  for (size_t i = 0; i < length; i++) {
    crc ^= (data[i] & 0x00FF);
    for (int bit = 0; bit < 8; bit++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

uint16_t ascii_to_uint16(const uint8_t *ascii_crc)
{
  char buffer[5] = {0};
  memcpy(buffer, ascii_crc, 4);
  return (uint16_t)strtol(buffer, NULL, 16);
}

void uint16_to_ascii_hex(uint16_t value, char *output)
{
  sprintf(output, "%04X", value);
}

uint16_t computeFrameCRC(const char *sender, const char *receiver,
                         const char *command, const char *data)
{
    char buf[256];
    int len = snprintf(buf, sizeof(buf), "%s%s%s%s",
                       sender, receiver, command, data);
    if (len < 0 || len >= (int)sizeof(buf)) {
        // obsługa błędu
        return 0;
    }
    return calculate_crc((uint8_t*)buf, (size_t)len);
}


/* ----------------- Logika komend ------------------------ */
void processCommand(const char *command, char *data)
{

    memset(data, 0, 100);


    if ( (memcmp(command, "STOP", 4) == 0) && (command[4] == '\0') )
    {

        gameTimerState = -1;
        sprintf(data, "PAUSED");
    }


    else if ( (memcmp(command, "RESUME", 6) == 0) && (command[6] == '\0') )
    {

        gameTimerState = 1;
        sprintf(data, "STARTED");
    }

    else if ( (memcmp(command, "BTNSTATE", 8) == 0) && (command[8] == '\0') )
    {

        GPIO_PinState pinState = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_10);


        if (pinState == GPIO_PIN_RESET)
        {

            sprintf(data, "BTN1");
        }
        else
        {

            sprintf(data, "BTN0");
        }
    }


    else if ( (memcmp(command, "POS", 3) == 0) && (command[3] == '\0') )
    {

        sprintf(data, "X%uY%u", (unsigned int)joystick[0], (unsigned int)joystick[1]);
    }


    else
    {

        sprintf(data, "INVALID_COMMAND");
    }
}
void sendResponse(const char *sender, const char *receiver,
                  const char *command, const char *data)
{

    uint16_t crcVal = computeFrameCRC(sender, receiver, command, data);
    char crcAscii[5];
    uint16_to_ascii_hex(crcVal, crcAscii);


    char frameCore[220];

    int coreLen = snprintf(frameCore, sizeof(frameCore),
                           "%s%s%s%s", sender, receiver, command, data);


    if (coreLen < 0 || coreLen > (FRAME_MAX_LENGTH - 6))
    {

        return;
    }


    char fullFrame[250];
    int ffLen = snprintf(fullFrame, sizeof(fullFrame),
                         "[%s%s]", frameCore, crcAscii);
    if (ffLen < 0 || ffLen >= (int)sizeof(fullFrame))
    {

        return;
    }


    USART_fsend("%s", fullFrame);
}

void sendErrorResponse(const char *receiver, const char *sender, const char *errorCode)
{

    sendResponse(receiver, sender, errorCode, "");
}



void handleFrame(const uint8_t *frame, size_t length)
{
    if (length < 10)
    {

        return;
    }


    char asciiCrc[5];
    memcpy(asciiCrc, &frame[length - 5], 4);
    asciiCrc[4] = '\0';


    int coreLen = (int)length - 6;
    if (coreLen < 0)
    {
        return;
    }


    char coreBuf[256];
    memcpy(coreBuf, &frame[1], coreLen);
    coreBuf[coreLen] = '\0'; // NUL-terminacja


    char sender[3];
    memcpy(sender, coreBuf, 2);
    sender[2] = '\0';


    char receiver[3];
    memcpy(receiver, &coreBuf[2], 2);
    receiver[2] = '\0';


    if (memcmp(receiver, "MC", 2) != 0)
    {

        sendErrorResponse("MC", sender, "INVALID_RECEIVER");
        return;
    }


    const char *pCmdData = &coreBuf[4];


    char command[11];
    int i = 0;
    while ((pCmdData[i] >= 'A') && (pCmdData[i] <= 'Z') && (i < 10))
    {
        command[i] = pCmdData[i];
        i++;
    }
    command[i] = '\0';


    const char *data = "";


    uint16_t calcCrc = computeFrameCRC(sender, receiver, command, data);

    char calcCrcStr[5];
    uint16_to_ascii_hex(calcCrc, calcCrcStr);


    if (memcmp(asciiCrc, calcCrcStr, 4) != 0)
    {
        sendErrorResponse("MC", sender, "INVALID_CRC");
        return;
    }


    char outData[100];
    processCommand(command, outData);


    sendResponse(receiver, sender, command, outData);
}


uint8_t* getFrame(size_t *outLength)
{
    static uint8_t  frameBuf[FRAME_MAX_LENGTH];
    static uint16_t frameLen       = 0;
    static FrameState frameState   = WAITING_FOR_FRAME;

    while (USART_kbhit())
    {
        char c = (char)USART_getchar();
        if (c < 0)
        {
            // Błąd/brak znaku
            return NULL;
        }

        switch (frameState)
        {
        case WAITING_FOR_FRAME:
            // Czekamy na '[' aby rozpocząć ramkę
            if (c == '[')
            {
                frameState    = COLLECTING_FRAME;
                frameLen      = 0;
                frameBuf[frameLen++] = '[';
            }
            // Inne znaki poza ramką ignorujemy
            break;

        case COLLECTING_FRAME:
            if (c == '\\')
            {
                // Używamy sekwencji escape
                frameState = ESCAPING_CHARACTER;
            }
            else if (c == '[')
            {
                // Nowy '[' w trakcie ramki => reset i zaczynamy od nowa
                frameLen = 0;
                frameBuf[frameLen++] = '[';
            }
            else if (c == ']')
            {
                // Koniec ramki
                if (frameLen < FRAME_MAX_LENGTH - 1)
                {
                    frameBuf[frameLen++] = ']';
                    frameBuf[frameLen]   = '\0';  // Terminator
                    *outLength           = frameLen;

                    // Reset stanu do następnej ramki
                    frameState = WAITING_FOR_FRAME;
                    return frameBuf;  // Zwracamy gotową ramkę
                }
                else
                {
                    // Przepełnienie -> porzucamy ramkę
                    frameState = WAITING_FOR_FRAME;
                    frameLen   = 0;
                }
            }
            else
            {
                // Zwykły znak wewnątrz ramki
                if (frameLen < FRAME_MAX_LENGTH - 1)
                {
                    frameBuf[frameLen++] = (uint8_t)c;
                }
                else
                {
                    // Przepełnienie
                    frameState = WAITING_FOR_FRAME;
                    frameLen   = 0;
                }
            }
            break;

        case ESCAPING_CHARACTER:
        {
            // Interpretujemy znak po '\'
            char decoded;
            switch (c)
            {
            case '1':  decoded = '['; break;
            case '2':  decoded = ']'; break;
            case '3':  decoded = '\\'; break;
            default:
                // Nieznana sekwencja -> porzucamy ramkę
                frameState = WAITING_FOR_FRAME;
                frameLen   = 0;
                continue;
            }

            // Wracamy do COLLECTING_FRAME
            frameState = COLLECTING_FRAME;

            // Dodajemy zdekodowany znak do bufora
            if (frameLen < FRAME_MAX_LENGTH - 1)
            {
                frameBuf[frameLen++] = (uint8_t)decoded;
            }
            else
            {
                // Przepełnienie
                frameState = WAITING_FOR_FRAME;
                frameLen   = 0;
            }
            break;
        }

        default:
            // Bezpieczny reset
            frameState = WAITING_FOR_FRAME;
            frameLen   = 0;
            break;
        }
    }

    // Brak pełnej ramki - NULL
    return NULL;
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

#ifdef  USE_FULL_ASSERT
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
