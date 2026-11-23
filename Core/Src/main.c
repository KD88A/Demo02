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
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal_tim.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct{
  GPIO_TypeDef *GPION;
  uint32_t PIN;
}PINPACK;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */
PINPACK K0 = {GPIOE,GPIO_PIN_4};
PINPACK K1 = {GPIOE,GPIO_PIN_3};
PINPACK LED1 = {GPIOA,GPIO_PIN_7};
osSemaphoreId_t sema1;
osThreadId_t keyprocess_handle;
osThreadId_t taskmanager_handle;
osThreadId_t task01_handle;
osThreadId_t task02_handle;
osThreadId_t task03_handle;
osThreadId_t task04_handle;
osThreadId_t task05_handle;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM6_Init(void);
static void MX_ADC1_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */
static void keyprocess(void *pv);
static void taskmanager(void *pv);
static void task01(void *pv);
static void task02(void *pv);
static void task03(void *pv);
static void task04(void *pv);
static void task05(void *pv);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile uint32_t PWM_1[] = {250,500,750,1000};
volatile uint8_t PWM_TYPE_1 = 0;
static int32_t times_k0 = -1;
static int32_t times_k1 = -1;

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
  MX_TIM3_Init();
  MX_TIM2_Init();
  MX_TIM6_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, PWM_1[PWM_TYPE_1]);
  
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  sema1 = osSemaphoreNew(5, 0, NULL);
  
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  static const osThreadAttr_t attr_high = {
    .priority = osPriorityAboveNormal,
  };
  keyprocess_handle = osThreadNew(keyprocess, NULL, &attr_high);
  taskmanager_handle = osThreadNew(taskmanager, NULL, &attr_high);
  task01_handle = osThreadNew(task01, NULL, NULL);
  task02_handle = osThreadNew(task02, NULL, NULL);
  task03_handle = osThreadNew(task03, NULL, NULL);
  task04_handle = osThreadNew(task04, NULL, NULL);
  task05_handle = osThreadNew(task05, NULL, NULL);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
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
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 19999;
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
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 8399;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 65535;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);

  /*Configure GPIO pins : PE3 PE4 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 15, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
  
}
//taskmanager函数：根据k0被按下的次数切换任务，防止按键被其他函数处理
static void taskmanager(void *pv){
  osThreadSuspend(task01_handle);
  osThreadSuspend(task02_handle);
  osThreadSuspend(task03_handle);
  osThreadSuspend(task04_handle);
  osThreadSuspend(task05_handle);
  uint8_t status = 0;
  uint8_t key = 0;
  
  for(;;){
    if (times_k0 > 0) {
    key = 1;
    }
    if (times_k0 == -1 && key == 1) {
    key = 2;
    }
    if (key == 2) {
      key = 0;
      status++;
      if (status > 5) {
      status = 1;
      }
      if (status == 1) {
      HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4); 
    osThreadSuspend(task05_handle);
    osThreadResume(task01_handle);
    }else if(status == 2){
    HAL_TIM_Base_Stop_IT(&htim6);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    osThreadSuspend(task01_handle);
    osThreadResume(task02_handle);
    }else if(status == 3){
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);  
    osThreadSuspend(task02_handle);
    osThreadResume(task03_handle);
    }else if(status == 4){
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1,0);
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    osThreadSuspend(task03_handle);
    osThreadResume(task04_handle);
    }else if(status == 5){
      HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
      HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    osThreadSuspend(task04_handle);
    osThreadResume(task05_handle);
    }
    
    }
    osDelay(20);
    }
    
}
static void task01(void *pv){
  uint32_t t = 0;
  uint8_t status = 0;
  uint8_t repeat = 0;
  __HAL_TIM_SET_AUTORELOAD(&htim6, 999);
  for(;;){
    if(times_k1 > 0){
    t = times_k1;
  }
  if(times_k1 == -1){
    if(t>0 && t<=25){
      HAL_GPIO_TogglePin(LED1.GPION, LED1.PIN);
    }else if (t>25) {
      if (status == 1) {
      status = 2;
      repeat = 0;
      }else if(status == 2 || status == 0){
      status = 1;
      repeat = 0;
      }
      
    }
    t = 0;
  }
  
  if(status == 1){
    if (repeat == 0) {
      HAL_TIM_Base_Start_IT(&htim6);
      repeat = 1;
    }
    
  }else if(status == 2){
    if (repeat == 0) {
      HAL_TIM_Base_Stop_IT(&htim6);
      __HAL_TIM_SET_COUNTER(&htim6, 0);
      repeat = 1;
    }
  }
  osDelay(5);
  }
  
}
static void task02(void *pv){
  uint8_t status = 0;
  uint32_t t1 = 0;
  uint32_t t2 = 500;
  for (;;) {
  
  osDelay(1);
  if(status == 0){
    if(t1<999){
      t1 = t1 + 1;
    }
    if (t2<2499) {
      t2 = t2 + 2;
    }
  }else if(status ==1 ){
    if (t1>0) {
    t1 = t1 - 1;
    }
    if (t2>500) {
    t2 = t2 - 2;
    }
  }
  if(t1 == 999){
    status = 1;
  }else if(t1 == 0){
    status = 0;
  }
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, t1);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, t2);
  }
}
static void task03(void *pv){
  uint32_t raw = 0;
  for (;;) {
  if (HAL_ADC_Start(&hadc1) == HAL_OK) {
    if(HAL_ADC_PollForConversion(&hadc1,10) == HAL_OK){
      raw = HAL_ADC_GetValue(&hadc1);
    }
  }
  HAL_ADC_Stop(&hadc1);
  osDelay(20);
  }
}
static void task04(void *pv){
  uint32_t t = 0;
  uint8_t status = 0;
  uint8_t repeat = 0;
  uint8_t status2 = 0;
  uint32_t t1 = 0;
  uint8_t status3 = 0;
  __HAL_TIM_SET_AUTORELOAD(&htim6, 999);
  for(;;){
    if(times_k1 > 0){
    t = times_k1;
  }
  if(times_k1 == -1){
    if(t>0 && t<=25){
      status2++;
      if (status2 > 3) {
      status2 = 0;
      }
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, PWM_1[status2]);
    }else if (t>25) {
      if (status == 1) {
      status = 2;
      repeat = 0;
      }else if(status == 2 || status == 0){
      status = 1;
      repeat = 0;
      }
      
    }
    t = 0;
  }
  
  if(status == 1){
    /*if (repeat == 0) {
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
      repeat = 1;
    }*/
    if(status3 == 0){
    if(t1<999){
      t1 = t1 + 1;
    }
    
  }else if(status3 ==1 ){
    if (t1>0) {
    t1 = t1 - 1;
    }
    
  }
  if(t1 == 999){
    status3 = 1;
  }else if(t1 == 0){
    status3 = 0;
  }
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, t1);
  
  }
  /*if(status == 2){
    if (repeat == 0) {
      HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
      repeat = 1;
    }
  }*/
  osDelay(1);
  }
  
}
static void task05(void *pv){
  uint32_t raw = 0;
  uint32_t angle = 0;
  for (;;) {
  if (HAL_ADC_Start(&hadc1) == HAL_OK) {
    if(HAL_ADC_PollForConversion(&hadc1,10) == HAL_OK){
      raw = HAL_ADC_GetValue(&hadc1);
      HAL_ADC_Stop(&hadc1);
    angle = raw*1000/2048 + 500;
    if(angle>500 && angle<2499){
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, angle);
    }
    }
  }
  
  osDelay(20);
  }
}
//keyproces函数：以20ms为周期轮询按键，t>20ms时times被置为1以达到消抖效果，反复叠加达到计时效果
//(这何尝不是一种非阻塞呢)

static void keyprocess(void *pv){
  for (; ; ) {
    



    osDelay(20);
    if(HAL_GPIO_ReadPin(K0.GPION, K0.PIN) == GPIO_PIN_RESET){
      times_k0++;
    }
    if(HAL_GPIO_ReadPin(K0.GPION, K0.PIN) == GPIO_PIN_SET){
      times_k0 = -1;
    }
    if(HAL_GPIO_ReadPin(K1.GPION, K1.PIN) == GPIO_PIN_RESET){
      times_k1++;
    }
    if(HAL_GPIO_ReadPin(K1.GPION, K1.PIN) == GPIO_PIN_SET){
      times_k1 = -1;
    }

  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM7 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM7)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if(htim->Instance == TIM3){

  }
  if(htim->Instance == TIM6){
    HAL_GPIO_TogglePin(LED1.GPION, LED1.PIN);
  }
    
  /* USER CODE END Callback 1 */
}

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
