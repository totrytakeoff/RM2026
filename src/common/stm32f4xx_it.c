/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "can.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

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
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line0 interrupt.
  */
void EXTI0_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI0_IRQn 0 */

  /* USER CODE END EXTI0_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
  /* USER CODE BEGIN EXTI0_IRQn 1 */

  /* USER CODE END EXTI0_IRQn 1 */
}

/**
  * @brief This function handles EXTI line1 interrupt.
  */
void EXTI1_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI1_IRQn 0 */

  /* USER CODE END EXTI1_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
  /* USER CODE BEGIN EXTI1_IRQn 1 */

  /* USER CODE END EXTI1_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles CAN1 RX0 interrupts.
  */
void CAN1_RX0_IRQHandler(void)
{
  /* USER CODE BEGIN CAN1_RX0_IRQn 0 */

  /* USER CODE END CAN1_RX0_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan1);
  /* USER CODE BEGIN CAN1_RX0_IRQn 1 */

  /* USER CODE END CAN1_RX0_IRQn 1 */
}

/**
  * @brief This function handles CAN2 RX0 interrupts.
  */
void CAN2_RX0_IRQHandler(void)
{
  /* USER CODE BEGIN CAN2_RX0_IRQn 0 */

  /* USER CODE END CAN2_RX0_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan2);
  /* USER CODE BEGIN CAN2_RX0_IRQn 1 */

  /* USER CODE END CAN2_RX0_IRQn 1 */
}

/**
  * @brief This function handles OTG_FS global interrupt.
  * @details USB 中断处理函数 - 必须存在否则 USB 枚举会失败
  *          这个中断处理器响应 USB 设备的所有事件，包括：
  *          - 数据接收 (OUT tokens)
  *          - 数据发送完成 (IN tokens)
  *          - 总线重置
  *          - 挂起/恢复等
  */
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

void OTG_FS_IRQHandler(void)
{
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}

/* USER CODE BEGIN 1 */

// 声明外部C++类和函数
#ifdef __cplusplus
extern "C" {
#endif

// SerialPort类的前向声明和访问函数
typedef struct SerialPort SerialPort;
SerialPort* getSerialPort(int index);

// SerialPort类的公共方法（通过友元访问）
void SerialPort_handleIRQ(SerialPort* port);
void SerialPort_rxCompleteCallback(SerialPort* port);
void SerialPort_txCompleteCallback(SerialPort* port);
void SerialPort_errorCallback(SerialPort* port);
UART_HandleTypeDef* SerialPort_getUartHandle(SerialPort* port);
DMA_HandleTypeDef* SerialPort_getDmaTxHandle(SerialPort* port);
DMA_HandleTypeDef* SerialPort_getDmaRxHandle(SerialPort* port);

#ifdef __cplusplus
}
#endif

/**
 * @brief USART1中断服务函数
 */
void USART1_IRQHandler(void)
{
    SerialPort* port = getSerialPort(0);
    if (port) {
        SerialPort_handleIRQ(port);
    }
}

/**
 * @brief USART6中断服务函数
 */
void USART6_IRQHandler(void)
{
    SerialPort* port = getSerialPort(3);
    if (port) {
        SerialPort_handleIRQ(port);
    }
}

/**
 * @brief DMA2 Stream7中断服务函数 (USART1 TX)
 */
void DMA2_Stream7_IRQHandler(void)
{
    SerialPort* port = getSerialPort(0);
    if (port) {
        DMA_HandleTypeDef* hdma = SerialPort_getDmaTxHandle(port);
        if (hdma) {
            HAL_DMA_IRQHandler(hdma);
        }
    }
}

/**
 * @brief DMA2 Stream2中断服务函数 (USART1 RX)
 */
void DMA2_Stream2_IRQHandler(void)
{
    SerialPort* port = getSerialPort(0);
    if (port) {
        DMA_HandleTypeDef* hdma = SerialPort_getDmaRxHandle(port);
        if (hdma) {
            HAL_DMA_IRQHandler(hdma);
        }
    }
}

/**
 * @brief DMA2 Stream6中断服务函数 (USART6 TX)
 */
void DMA2_Stream6_IRQHandler(void)
{
    SerialPort* port = getSerialPort(3);
    if (port) {
        DMA_HandleTypeDef* hdma = SerialPort_getDmaTxHandle(port);
        if (hdma) {
            HAL_DMA_IRQHandler(hdma);
        }
    }
}

/**
 * @brief DMA2 Stream1中断服务函数 (USART6 RX)
 */
void DMA2_Stream1_IRQHandler(void)
{
    SerialPort* port = getSerialPort(3);
    if (port) {
        DMA_HandleTypeDef* hdma = SerialPort_getDmaRxHandle(port);
        if (hdma) {
            HAL_DMA_IRQHandler(hdma);
        }
    }
}

/**
 * @brief HAL库UART接收完成回调
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    for (int i = 0; i < 4; i++) {
        SerialPort* port = getSerialPort(i);
        if (port && SerialPort_getUartHandle(port) == huart) {
            SerialPort_rxCompleteCallback(port);
            break;
        }
    }
}

/**
 * @brief HAL库UART发送完成回调
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    for (int i = 0; i < 4; i++) {
        SerialPort* port = getSerialPort(i);
        if (port && SerialPort_getUartHandle(port) == huart) {
            SerialPort_txCompleteCallback(port);
            break;
        }
    }
}

/**
 * @brief HAL库UART错误回调
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    for (int i = 0; i < 4; i++) {
        SerialPort* port = getSerialPort(i);
        if (port && SerialPort_getUartHandle(port) == huart) {
            SerialPort_errorCallback(port);
            break;
        }
    }
}

// 声明外部C函数（用于处理Button中断）
extern void button_interrupt_handler(uint16_t GPIO_Pin);

/**
  * @brief  GPIO外部中断回调函数
  * @param  GPIO_Pin: 触发中断的引脚
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // 调用外部的Button中断处理函数
    button_interrupt_handler(GPIO_Pin);
}

/* USER CODE END 1 */
