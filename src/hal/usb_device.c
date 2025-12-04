/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usb_device.c
  * @version        : v1.0_Cube
  * @brief          : USB设备实现文件 - 负责STM32 USB CDC虚拟串口的初始化和配置
  * @details        :
  *     本文件实现USB设备的完整初始化流程，包括：
  *     1. USB设备核心库初始化
  *     2. CDC(通信设备类)注册 - 实现虚拟串口功能
  *     3. CDC接口回调函数注册
  *     4. USB设备启动
  *
  *     USB CDC虚拟串口使得STM32可以通过USB连接到PC，
  *     在PC上显示为一个串口设备，可以进行双向数据通信
  ******************************************************************************
  */

/* Includes -----------------------------------------------------------------*/
#include "usb_device.h"     // USB设备配置头文件
#include "usbd_core.h"      // USB核心库头文件 - 提供USB设备的核心功能
#include "usbd_desc.h"      // USB描述符头文件 - 定义设备的各种描述符
#include "usbd_cdc.h"       // USB CDC类头文件 - 通信设备类的标准实现
#include "usbd_cdc_if.h"    // USB CDC接口头文件 - 自定义的CDC接口实现
#include "main.h"           // 主头文件 - 包含Error_Handler声明

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USB设备核心句柄声明 - 全局变量，存储USB设备的状态和配置信息 */
USBD_HandleTypeDef hUsbDeviceFS;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * @brief  USB设备库初始化函数 - 完成USB CDC虚拟串口的完整初始化
  * @details 初始化步骤：
  *          1. 初始化USB设备核心库
  *          2. 注册CDC类（实现虚拟串口功能）
  *          3. 注册CDC接口回调函数（处理数据收发）
  *          4. 启动USB设备开始工作
  * @retval None
  */
void MX_USB_DEVICE_Init(void)
{
  /* USER CODE BEGIN USB_DEVICE_Init_PreTreatment */
  // USB初始化前的预处理代码区域
  /* USER CODE END USB_DEVICE_Init_PreTreatment */

  /* 第一步：初始化USB设备核心库 */
  // 参数说明：
  // &hUsbDeviceFS - USB设备句柄指针
  // &FS_Desc - USB描述符结构体（设备描述符、配置描述符等）
  // DEVICE_FS - 全速设备模式
  if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK) {
    Error_Handler();  // 初始化失败则调用错误处理函数
  }

  /* 第二步：注册CDC类（通信设备类） */
  // CDC类使STM32在PC上显示为一个虚拟串口设备
  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC) != USBD_OK) {
    Error_Handler();
  }

  /* 第三步：注册CDC接口回调函数 */
  // USBD_Interface_fops_FS定义了CDC接口的具体操作函数
  // 包括初始化、数据接收、数据发送等回调函数
  if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK) {
    Error_Handler();
  }

  /* 第四步：启动USB设备 */
  // 启动后USB设备开始响应主机的枚举请求
  if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
    Error_Handler();
  }

  /* USER CODE BEGIN USB_DEVICE_Init_PostTreatment */
  // USB初始化后的处理代码区域
  /* USER CODE END USB_DEVICE_Init_PostTreatment */
}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

/**
  * @}
  */

/**
  * @}
  */
