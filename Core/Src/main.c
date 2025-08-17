

#include "main.h"
#include "cmsis_os.h"

/* =========================
 * FreeRTOS Event Flags
 * ========================= */
#define FLAG_I2C_DONE     (1u << 0)
#define FLAG_SPI_DONE     (1u << 1)
#define FLAG_CAN_DONE     (1u << 2)
#define FLAG_UART_DONE    (1u << 3)

/* =========================
 * HAL Handle Definitions
 * (stm32f4xx_it.c 에서 extern 으로 참조)
 * ========================= */
ADC_HandleTypeDef   hadc1;
CAN_HandleTypeDef   hcan1;

I2C_HandleTypeDef   hi2c1;
I2C_HandleTypeDef   hi2c2;
DMA_HandleTypeDef   hdma_i2c1_rx;
DMA_HandleTypeDef   hdma_i2c1_tx;
DMA_HandleTypeDef   hdma_i2c2_rx;
DMA_HandleTypeDef   hdma_i2c2_tx;

SPI_HandleTypeDef   hspi1;
SPI_HandleTypeDef   hspi2;
DMA_HandleTypeDef   hdma_spi1_rx;
DMA_HandleTypeDef   hdma_spi1_tx;
DMA_HandleTypeDef   hdma_spi2_rx;
DMA_HandleTypeDef   hdma_spi2_tx;

UART_HandleTypeDef  huart4;

/* =========================
 * RTOS Kernel Objects
 * (필요 시 tasks.c 에서 extern 로 참조)
 * ========================= */
osEventFlagsId_t    CommEventFlagHandle;
osMutexId_t         CommMutexHandleHandle;
osMessageQueueId_t  CanQueueHandle;

/* =========================
 * RTOS Thread Handles
 * ========================= */
osThreadId_t defaultTaskHandle;
osThreadId_t I2CTaskHandle;
osThreadId_t SPITaskHandle;
osThreadId_t CANTaskHandle;
osThreadId_t UARTTaskHandle;

/* =========================
 * Function Prototypes
 * ========================= */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_CAN1_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_UART4_Init(void);

/* =========================
 * Application entry
 * ========================= */
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_CAN1_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_UART4_Init();

  // === RTOS 커널 초기화 ===
  osKernelInitialize();

  // === 커널 객체 생성 ===
  CommEventFlagHandle     = osEventFlagsNew(NULL);
  CommMutexHandleHandle   = osMutexNew(NULL);
  CanQueueHandle          = osMessageQueueNew(8, 8, NULL);

  // === Task 생성 (엔트리 함수는 tasks.c 에 구현) ===
  const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask", .stack_size = 128 * 4, .priority = (osPriority_t)osPriorityNormal,
  };
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  const osThreadAttr_t I2CTask_attributes = {
    .name = "I2CTask", .stack_size = 128 * 4, .priority = (osPriority_t)osPriorityNormal,
  };
  I2CTaskHandle = osThreadNew(StartI2CTask, NULL, &I2CTask_attributes);

  const osThreadAttr_t SPITask_attributes = {
    .name = "SPITask", .stack_size = 128 * 4, .priority = (osPriority_t)osPriorityNormal,
  };
  SPITaskHandle = osThreadNew(StartSPITask, NULL, &SPITask_attributes);

  const osThreadAttr_t CANTask_attributes = {
    .name = "CANTask", .stack_size = 128 * 4, .priority = (osPriority_t)osPriorityNormal,
  };
  CANTaskHandle = osThreadNew(StartCANTask, NULL, &CANTask_attributes);

  const osThreadAttr_t UARTTask_attributes = {
    .name = "UARTTask", .stack_size = 128 * 4, .priority = (osPriority_t)osPriorityNormal,
  };
  UARTTaskHandle = osThreadNew(StartUARTTask, NULL, &UARTTask_attributes);

  // === RTOS 시작 ===
  osKernelStart();

  // 실행은 각 Task 가 담당
  while (1) { }
}

/* =========================
 * Clock / Periph Inits
 * ========================= */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) { Error_Handler(); }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

static void MX_DMA_Init(void)
{
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  // FreeRTOS 호환: 우선순위 5 (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 보다 낮게)
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

  HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);

  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);

  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

  HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);

  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

  // 필요 시만 0 우선순위 사용 (타이밍 중요 DMA) — FreeRTOS API 호출 금지 ISR
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
}

static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode          = DISABLE;
  hadc1.Init.ContinuousConvMode    = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion       = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) { Error_Handler(); }

  sConfig.Channel      = ADC_CHANNEL_2;
  sConfig.Rank         = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) { Error_Handler(); }
}

static void MX_CAN1_Init(void)
{
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler           = 16;
  hcan1.Init.Mode                = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth       = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1            = CAN_BS1_1TQ;
  hcan1.Init.TimeSeg2            = CAN_BS2_1TQ;
  hcan1.Init.TimeTriggeredMode   = DISABLE;
  hcan1.Init.AutoBusOff          = DISABLE;
  hcan1.Init.AutoWakeUp          = DISABLE;
  hcan1.Init.AutoRetransmission  = DISABLE;
  hcan1.Init.ReceiveFifoLocked   = DISABLE;
  hcan1.Init.TransmitFifoPriority= DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK) { Error_Handler(); }

  // CAN IRQ 활성화 (stm32f4xx_it.c 에서 HAL_CAN_IRQHandler 사용)
  HAL_NVIC_SetPriority(CAN1_TX_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
  HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance             = I2C1;
  hi2c1.Init.ClockSpeed      = 100000;
  hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1     = 0;
  hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2     = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) { Error_Handler(); }

  HAL_NVIC_SetPriority(I2C1_EV_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
  HAL_NVIC_SetPriority(I2C1_ER_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
}

static void MX_I2C2_Init(void)
{
  hi2c2.Instance             = I2C2;
  hi2c2.Init.ClockSpeed      = 100000;
  hi2c2.Init.DutyCycle       = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1     = 0;
  hi2c2.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2     = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK) { Error_Handler(); }

  HAL_NVIC_SetPriority(I2C2_EV_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);
}

static void MX_SPI1_Init(void)
{
  hspi1.Instance               = SPI1;
  hspi1.Init.Mode              = SPI_MODE_MASTER;
  hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
  hspi1.Init.NSS               = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial     = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) { Error_Handler(); }

  HAL_NVIC_SetPriority(SPI1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(SPI1_IRQn);
}

static void MX_SPI2_Init(void)
{
  hspi2.Instance               = SPI2;
  hspi2.Init.Mode              = SPI_MODE_MASTER;
  hspi2.Init.Direction         = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize          = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity       = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase          = SPI_PHASE_1EDGE;
  hspi2.Init.NSS               = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode            = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial     = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK) { Error_Handler(); }

  HAL_NVIC_SetPriority(SPI2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(SPI2_IRQn);
}

static void MX_UART4_Init(void)
{
  huart4.Instance        = UART4;
  huart4.Init.BaudRate   = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits   = UART_STOPBITS_1;
  huart4.Init.Parity     = UART_PARITY_NONE;
  huart4.Init.Mode       = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK) { Error_Handler(); }

  HAL_NVIC_SetPriority(UART4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(UART4_IRQn);
}

/* =========================
 * Error Handler
 * ========================= */
void Error_Handler(void)
{
  __disable_irq();
  while (1) { }
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
