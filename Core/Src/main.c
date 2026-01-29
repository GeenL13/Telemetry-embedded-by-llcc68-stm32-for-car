/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *s.
  * All rights reserved.
  * Copyright (c) 2025 STMicroelectronic
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
#include "can.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "llcc68.h"
#include "llcc68_app.h"
#include "Can_Config.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define CAN_FRAME_BUFFER_SIZE 20  // CAN数据帧缓冲区大小

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// 配置
#define MODEL	1	// 收发模式   1：发送；0：接收



/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// CAN数据帧
typedef struct __attribute__((__packed__))
{
    uint8_t  frame_start_1; // 帧头: 0x55
    uint8_t  frame_start_2; // 帧头: 0xAA
    uint8_t  ide;        // 0: 标准帧, 1: 扩展帧
    uint8_t  rtr;        // 0: 数据帧, 1: 远程帧
    uint8_t  dlc;        // 0~8
    uint8_t  reserved;   // 保留位
    uint32_t id;         // ide=0 时，仅低 11 bit 有效；ide=1 时，低 29 bit 有效
    uint8_t  data[8];    // 实际用前 dlc 个字节
    uint16_t crc;       // CRC 校验码
} can_frame_t;

// CAN数据帧环形缓冲区
can_frame_t can_frame_buffer[CAN_FRAME_BUFFER_SIZE] = {0};
uint8_t can_frame_current_send_index = 0;
uint8_t can_frame_current_receive_index = 0;

// 必须添加 volatile，防止编译器优化导致主循环无法读取到中断更新的值
volatile uint8_t can_frame_buffer_send_count = 0;
volatile uint8_t can_frame_buffer_receive_count = 0;

// 接收缓存区
uint8_t llcc68_rx_buffer[128] = {0};


// 中断计时变量
uint32_t tick = 0;
uint32_t pb12_tick = 0;
uint32_t pb13_tick = 0;
uint32_t pb14_tick = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

// 计算CRC16校验码
uint16_t Calculate_CRC16(uint8_t *pData, uint16_t len);

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
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_CAN_Init();
  /* USER CODE BEGIN 2 */

  // 初始化定时器中断
  HAL_TIM_Base_Start_IT(&htim1);
  // 初始化LLCC68
  LLCC68_Init(MODEL);
  // 初始化Can
  if (MODEL == 1)
  {
	  CAN_Init(&hcan);
	  CAN_Filter_Config(&hcan, CAN_FILTER(13) | CAN_FIFO_1 | CAN_STDID | CAN_DATA_TYPE, 0xC0, 0x00);	// 初始化Can接收
  }


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 检测发送标志位以及发送数据
    if (can_frame_buffer_send_count != can_frame_buffer_receive_count && MODEL == 1)
    {
      uint8_t index = can_frame_current_send_index;
      // 填入帧头
      can_frame_buffer[index].frame_start_1 = 0x55;
      can_frame_buffer[index].frame_start_2 = 0xAA;
      // 填入保留字节
      can_frame_buffer[index].reserved = 0;
      // 计算crc
      can_frame_buffer[index].crc = Calculate_CRC16((uint8_t*)&can_frame_buffer[index], sizeof(can_frame_t) - 2);
      // 无线发送数据
      LLCC68_Send((uint8_t*)&can_frame_buffer[index], sizeof(can_frame_t));
      // 发送成功指示灯
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, SET);
      pb12_tick = tick;
      // 更新索引和计数
      can_frame_current_send_index = (can_frame_current_send_index + 1) % CAN_FRAME_BUFFER_SIZE;
      can_frame_buffer_send_count++;
    }


    // 正常运行好指示灯
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (can_frame_buffer_send_count == can_frame_buffer_receive_count)
    {
        HAL_Delay(1); 
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

// 计算CRC16校验码
// 数据指针， 数据长度
uint16_t Calculate_CRC16(uint8_t *pData, uint16_t len)
{
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++)
  {
    crc ^= (uint16_t)pData[i] << 8;
    for (uint8_t j = 0; j < 8; j++)
    {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021;
      else
        crc <<= 1;
    }
  }
  return crc;
}



// DIO1中断回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == DIO1_Pin)
  {
    uint16_t irq_status;
    llcc68_get_irq_status(NULL, &irq_status);
    LLCC68_Wait_Busy();
    if (irq_status == LLCC68_IRQ_TX_DONE)
    {
      LLCC68_TransmitCallback();
    }
    else if (irq_status == LLCC68_IRQ_RX_DONE)
    {
      LLCC68_ReceiveCallback(llcc68_rx_buffer, sizeof(llcc68_rx_buffer));
    }
    else if (irq_status == LLCC68_IRQ_TIMEOUT)
    {
      // 处理超时事件
      llcc68_clear_irq_status(NULL, LLCC68_IRQ_TIMEOUT);
      // 重启接收模式
      LLCC68_Receive();
      // 超时指示灯
      //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, SET);
      //pb14_tick = tick;
    }
  }
}

// 定时器中断
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim1)
	{
		tick++;
		// Tx
		if ((tick - pb12_tick) >= 30 && pb12_tick != 0)
		{
			pb12_tick = 0;
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, RESET);
		}
		// Rx
		if ((tick - pb13_tick) >= 30 && pb13_tick != 0)
		{
			pb13_tick = 0;
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, RESET);
		}
		// TimeOut
		if ((tick - pb14_tick) >= 30 && pb14_tick != 0)
		{
			pb14_tick = 0;
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, RESET);
		}
	}
}

// 串口中断回调
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    // 传输完成后的处理代码
    if (huart == &huart1)
    {
        ;// 可以在这里添加传输完成后的操作
    }
}

// CAN接收中断回调：FIFO1
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  // 仅发射模式有效
  if (MODEL != 1)
  {
	  return;
  }

  // can数据存储变量
	CAN_RxHeaderTypeDef header;
	uint8_t data[8] = {0};

	// 从FIFO中读取数据
	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &header, data);

  // 如果接收缓冲区饱和则直接丢弃
  if ((uint8_t)(can_frame_buffer_receive_count - can_frame_buffer_send_count) >= CAN_FRAME_BUFFER_SIZE - 2)  //强制将结果作为uint类型，利用uint溢出特性可以计算差值，防止编译器自动把结果提升成int导致出现负数
  {
      return;
  }

	// 是目标ID则进行填充并置发送标志位
	switch (header.StdId)
	{
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0x1A:
      // 填充数据帧
      can_frame_buffer[can_frame_current_receive_index].ide = (header.IDE == CAN_ID_STD) ? 0 : 1;
      can_frame_buffer[can_frame_current_receive_index].rtr = (header.RTR == CAN_RTR_DATA) ? 0 : 1;
      can_frame_buffer[can_frame_current_receive_index].dlc = header.DLC;
      can_frame_buffer[can_frame_current_receive_index].id = (header.IDE == CAN_ID_STD) ? header.StdId : header.ExtId;
      memcpy(can_frame_buffer[can_frame_current_receive_index].data, data, 8);	// 此处应当固定8字节，以防内存界限出现错误
      // 更新索引和计数
      can_frame_current_receive_index = (can_frame_current_receive_index + 1) % CAN_FRAME_BUFFER_SIZE;
      
      can_frame_buffer_receive_count++;
			break;

		default:
			break;
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
