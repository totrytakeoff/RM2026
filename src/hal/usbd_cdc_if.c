/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"  // CDC接口头文件 - 自定义的CDC接口实现
#include "main.h"         // 主程序头文件 - 包含GPIO和HAL库定义
#include <ctype.h>        // 字符处理库 - 用于大小写转换
#include <string.h>       // 字符串处理库 - 用于字符串操作
#include <stdio.h>        // 标准输入输出库 - 用于字符串格式化

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PRIVATE_DEFINES */
/* USB CDC接收和发送缓冲区大小定义 */
#define APP_RX_DATA_SIZE  1024  // USB接收缓冲区大小 - 存储从PC接收到的数据
#define APP_TX_DATA_SIZE  1024  // USB发送缓冲区大小 - 存储要发送到PC的数据

/* USER CODE END PRIVATE_DEFINES */

/* Private variables ---------------------------------------------------------*/
/* USB数据收发缓冲区 - 用于USB虚拟串口的数据传输 */
uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];  // USB接收缓冲区 - 存储从PC接收到的数据
uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];  // USB发送缓冲区 - 存储要发送到PC的数据

/* USB设备句柄声明 - 指向全局USB设备实例 */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USB类集成 - 声明外部C++类访问函数 */
typedef struct USBPort USBPort;
extern USBPort* getUSBPortInstance(void);
extern void USBPort_rxCallback(USBPort* port, uint8_t* data, uint32_t len);
extern void USBPort_connectCallback(USBPort* port);
extern void USBPort_disconnectCallback(USBPort* port);

/* 使用模式选择：0=原始命令模式，1=USBPort类模式 */
#define USE_USB_PORT_CLASS 1

/* 命令处理相关变量 - 用于解析从PC接收的控制命令 */
#define CMD_BUFFER_SIZE 64                   // 命令缓冲区大小

#if !USE_USB_PORT_CLASS
static uint8_t cmd_buffer[CMD_BUFFER_SIZE];  // 命令缓冲区 - 存储接收到的命令字符
static uint16_t cmd_length = 0;              // 当前命令长度计数器
#endif

/* RGB LED控制命令格式定义 */
// 基础命令格式: "LED X\r\n" 其中X为0或1，控制红色LED的开关状态
// RGB命令格式: "RGB R G B\r\n" 其中R,G,B为0-255，控制RGB LED的亮度
#define LED_CMD_PREFIX "LED "          // LED单色命令前缀
#define LED_CMD_PREFIX_LEN 4           // LED前缀长度

#define RGB_CMD_PREFIX "RGB "          // RGB命令前缀
#define RGB_CMD_PREFIX_LEN 4           // RGB前缀长度

#define HELP_CMD "HELP"                // 帮助命令
#define STATUS_CMD "STATUS"            // 状态查询命令

/* USER CODE END PRIVATE_VARIABLES */

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */
/* 私有函数声明 - 仅在原始命令模式下使用 */
#if !USE_USB_PORT_CLASS
static void ProcessCommand(uint8_t* buf, uint16_t len);           // 主命令处理函数
static void SendHelpMessage(void);                                // 发送帮助信息
static void SendStatusMessage(void);                              // 发送LED状态信息
static void ProcessLEDCommand(uint8_t* cmd, uint16_t len);        // 处理单色LED命令
static void ProcessRGBCommand(uint8_t* cmd, uint16_t len);        // 处理RGB LED命令
static void SetSingleLED(GPIO_TypeDef* GPIO_Port, uint16_t GPIO_Pin, uint8_t state); // 设置单个LED状态
#endif
/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);

/* CDC接口操作函数结构体 - 定义CDC虚拟串口的各种回调函数
 * 这个结构体会被USB CDC核心调用，实现与具体应用的数据交互
 */
USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,     // CDC初始化函数
  CDC_DeInit_FS,   // CDC去初始化函数
  CDC_Control_FS,  // CDC控制请求处理函数
  CDC_Receive_FS   // CDC数据接收函数 - 处理从PC接收到的数据
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  初始化CDC媒体底层驱动
  * @details 这个函数在USB设备枚举成功后被调用，用于设置数据收发缓冲区
  *          1. 设置USB发送缓冲区
  *          2. 设置USB接收缓冲区
  *          3. 准备开始数据通信
  * @retval USBD_OK 如果所有操作成功，否则返回USBD_FAIL
  */
static int8_t CDC_Init_FS(void)
{
  /* USER CODE BEGIN 3 */
  /* 设置应用程序的数据收发缓冲区 */

  /* 设置USB发送缓冲区 - 用于向PC发送数据 */
  // 参数：USB设备句柄、发送缓冲区地址、初始发送长度(0表示没有数据要发送)
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);

  /* 设置USB接收缓冲区 - 用于接收从PC发送的数据 */
  // 参数：USB设备句柄、接收缓冲区地址
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
  
  /* 关键：启动第一次USB接收 - 准备接收来自PC的数据 */
  // 这个调用非常重要！没有它USB将无法接收任何数据
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);

  /* 通知USBPort：CDC接口已初始化（设备已配置） */
  USBPort* port = getUSBPortInstance();
  if (port) {
    USBPort_connectCallback(port);
  }

  return (USBD_OK);  // 返回成功状态
  /* USER CODE END 3 */
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(void)
{
  /* USER CODE BEGIN 4 */
  /* 通知USBPort：CDC接口去初始化（设备已断开） */
  USBPort* port = getUSBPortInstance();
  if (port) {
    USBPort_disconnectCallback(port);
  }
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  Manage the CDC class requests
  * @param  cmd: Command code
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  /* USER CODE BEGIN 5 */
  switch(cmd)
  {
    case CDC_SEND_ENCAPSULATED_COMMAND:
    /* Add your code here */
    break;

    case CDC_GET_ENCAPSULATED_RESPONSE:
    /* Add your code here */
    break;

    case CDC_SET_COMM_FEATURE:
    /* Add your code here */
    break;

    case CDC_GET_COMM_FEATURE:
    /* Add your code here */
    break;

    case CDC_CLEAR_COMM_FEATURE:
    /* Add your code here */
    break;

    case CDC_SET_LINE_CODING:
    /* Add your code here */
    break;

    case CDC_GET_LINE_CODING:
    /* Add your code here */
    break;

    case CDC_SET_CONTROL_LINE_STATE:
    /* Add your code here */
    break;

    case CDC_SEND_BREAK:
    /* Add your code here */
    break;    
    
    default:
    break;
  }
  
  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  USB CDC数据接收回调函数 - 处理从PC接收到的数据
  * @details 这个函数是整个USB LED控制的核心，负责：
  *          1. 接收PC通过虚拟串口发送的数据
  *          2. 解析数据中的命令（支持换行符分割）
  *          3. 调用命令处理函数执行具体操作
  *          4. 准备下一次数据接收
  *
  *          命令处理机制：
  *          - 支持多个字符连续接收，直到遇到换行符(\n 或 \r)
  *          - 命令长度限制为63字符（预留1字节给结束符）
  *          - 超长命令会触发错误响应
  *
  * @param  Buf: 接收数据缓冲区指针 - 指向从PC接收到的数据
  * @param  Len: 接收数据长度指针 - 指向接收数据字节数
  * @retval 操作结果：USBD_OK表示成功，USBD_FAIL表示失败
  */
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
  /* USER CODE BEGIN 6 */
  /* 确保USB底层下一次可以继续接收：始终使用固定的接收缓冲区 */
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);

#if USE_USB_PORT_CLASS
  /* 使用USBPort类模式 - 将数据传递给C++类处理 */
  USBPort* port = getUSBPortInstance();
  if (port) {
    USBPort_rxCallback(port, Buf, *Len);
  }
#else
  /* 原始命令处理模式 - 逐字节处理接收到的数据 */
  for (uint32_t i = 0; i < *Len; i++) {

    /* 检查命令结束符 - 换行符或回车符表示一个完整命令 */
    if (Buf[i] == '\n' || Buf[i] == '\r') {

      /* 如果命令缓冲区有数据，处理完整命令 */
      if (cmd_length > 0) {
        cmd_buffer[cmd_length] = '\0'; // 添加字符串结束符
        ProcessCommand(cmd_buffer, cmd_length); // 调用命令处理函数
        cmd_length = 0; // 重置命令长度，准备接收新命令
      }

    } else if (cmd_length < (CMD_BUFFER_SIZE - 1)) {

      /* 将字符添加到命令缓冲区（防止缓冲区溢出） */
      cmd_buffer[cmd_length++] = Buf[i];

    } else {

      /* 命令过长错误处理 */
      cmd_length = 0; // 重置命令缓冲区
      const char* error = "ERROR: Command too long\r\n"; // 错误提示信息
      CDC_Transmit_FS((uint8_t*)error, strlen(error)); // 发送错误响应
    }
  }
#endif

  /* 重新准备USB接收，为下一次数据传输做准备 */
  // 这个调用非常重要，它告诉USB底层可以接收下一个数据包
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);

  return (USBD_OK); // 返回操作成功
  /* USER CODE END 6 */
}

/**
  * @brief  USB CDC数据发送函数 - 通过USB向PC发送数据
  * @details 这个函数用于向PC发送响应数据或状态信息：
  *          1. 检查当前发送状态，避免数据冲突
  *          2. 设置发送缓冲区和数据长度
  *          3. 启动USB数据传输
  *
  *          使用注意事项：
  *          - 每次只能进行一次发送操作，直到发送完成
  *          - 返回USBD_BUSY时表示上次发送未完成，需要等待
  *
  * @param  Buf: 要发送的数据缓冲区指针
  * @param  Len: 要发送的数据长度（字节数）
  * @retval USBD_OK - 发送成功
  * @retval USBD_BUSY - 发送忙（上次发送未完成）
  * @retval USBD_FAIL - 发送失败
  */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 7 */

  /* 获取CDC类句柄，用于检查发送状态 */
  USBD_CDC_HandleTypeDef *hcdc = NULL;
  if (hUsbDeviceFS.pClassData != NULL) {
    hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
  }

  /* 如果CDC类句柄无效，返回失败，防止访问空指针 */
  if (hcdc == NULL) {
    return USBD_FAIL;
  }

  /* 检查发送状态 - 如果正在发送则返回忙状态 */
  if (hcdc->TxState != 0) {
    return USBD_BUSY; // 发送忙，需要等待当前发送完成
  }
  
  /* 🔥 关键修复：必须复制数据到UserTxBufferFS！
   * 因为Buf可能指向UserRxBufferFS，会被下一次接收覆盖！
   * USB CDC的发送是异步的，必须使用独立的发送缓冲区！
   */
  if (Len > APP_TX_DATA_SIZE) {
    Len = APP_TX_DATA_SIZE;  // 限制长度，防止缓冲区溢出
  }
  memcpy(UserTxBufferFS, Buf, Len);  // ← 复制数据到发送缓冲区

  /* 设置发送缓冲区和数据长度 */
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, Len);

  /* 启动USB数据包发送 */
  result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);

  /* USER CODE END 7 */
  return result; // 返回发送操作结果
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

#if !USE_USB_PORT_CLASS
/**
  * @brief  主命令处理函数 - 解析和执行从PC接收的控制命令
  * @details 这是一个功能丰富的LED控制系统，支持多种命令格式：
  *
  *          支持的命令格式：
  *          1. "LED 0/1" - 控制红色LED开关（兼容原始命令）
  *          2. "RGB R G B" - 控制RGB LED，R/G/B为0-255的数值
  *          3. "HELP" - 显示所有可用命令的帮助信息
  *          4. "STATUS" - 查询当前LED状态
  *
  *          命令特点：
  *          - 命令不区分大小写
  *          - 以换行符(\n)或回车符(\r)结束
  *          - 支持错误处理和帮助提示
  *
  * @param  buf: 命令缓冲区指针 - 指向完整的命令字符串
  * @param  len: 命令长度 - 命令字符串的字符个数
  */
static void ProcessCommand(uint8_t* buf, uint16_t len)
{
  /* 将命令转换为大写以支持大小写不敏感的命令解析 */
  for (uint16_t i = 0; i < len; i++) {
    buf[i] = toupper(buf[i]);
  }

  /* 帮助命令 - 显示所有可用命令 */
  if (strncmp((char*)buf, HELP_CMD, strlen(HELP_CMD)) == 0) {
    SendHelpMessage();
    return;
  }

  /* 状态查询命令 - 显示当前LED状态 */
  if (strncmp((char*)buf, STATUS_CMD, strlen(STATUS_CMD)) == 0) {
    SendStatusMessage();
    return;
  }

  /* RGB LED控制命令 - 支持RGB三色独立控制 */
  if (len > RGB_CMD_PREFIX_LEN &&
      strncmp((char*)buf, RGB_CMD_PREFIX, RGB_CMD_PREFIX_LEN) == 0) {
    ProcessRGBCommand(buf, len);
    return;
  }

  /* 单色LED控制命令 - 兼容原始的LED命令格式 */
  if (len > LED_CMD_PREFIX_LEN &&
      strncmp((char*)buf, LED_CMD_PREFIX, LED_CMD_PREFIX_LEN) == 0) {
    ProcessLEDCommand(buf, len);
    return;
  }

  /* 未知命令处理 */
  const char* error = "ERROR: Unknown command. Type 'HELP' for available commands\r\n";
  CDC_Transmit_FS((uint8_t*)error, strlen(error));
}

/**
  * @brief  发送帮助信息 - 显示所有可用的控制命令
  * @details 这个函数向PC发送详细的使用说明，包括：
  *          - 所有支持的命令格式
  *          - 命令参数说明
  *          - 使用示例
  */
static void SendHelpMessage(void)
{
  const char* help_msg =
    "\r\n=== USB LED Control Help ===\r\n"
    "Available commands:\r\n"
    "  LED 0/1     - Turn red LED OFF/ON\r\n"
    "  RGB R G B   - Set RGB LED brightness (0-255 each)\r\n"
    "  HELP        - Show this help message\r\n"
    "  STATUS      - Show current LED status\r\n"
    "\r\n"
    "Examples:\r\n"
    "  LED 1       - Turn red LED ON\r\n"
    "  RGB 255 0 0 - Set RED LED to full brightness\r\n"
    "  RGB 0 255 0 - Set GREEN LED to full brightness\r\n"
    "  RGB 0 0 255 - Set BLUE LED to full brightness\r\n"
    "  RGB 255 255 255 - Set all LEDs to white (full brightness)\r\n"
    "  RGB 0 0 0   - Turn OFF all LEDs\r\n"
    "\r\n"
    "Note: Commands are NOT case-sensitive\r\n"
    "==============================\r\n";

  CDC_Transmit_FS((uint8_t*)help_msg, strlen(help_msg));
}

/**
  * @brief  发送LED状态信息 - 显示当前所有LED的状态
  * @details 这个函数读取当前LED的GPIO状态并向PC发送状态报告
  */
static void SendStatusMessage(void)
{
  // 读取当前LED状态（注意：这里需要根据实际硬件配置调整）
  GPIO_PinState red_state = HAL_GPIO_ReadPin(LED_R_GPIO_Port, LED_R_Pin);
  GPIO_PinState green_state = HAL_GPIO_ReadPin(LED_G_GPIO_Port, LED_G_Pin);
  GPIO_PinState blue_state = HAL_GPIO_ReadPin(LED_B_GPIO_Port, LED_B_Pin);

  char status_msg[128];
  int len = snprintf(status_msg, sizeof(status_msg),
    "\r\n=== LED Status ===\r\n"
    "Red LED:   %s\r\n"
    "Green LED: %s\r\n"
    "Blue LED:  %s\r\n"
    "==================\r\n",
    (red_state == GPIO_PIN_SET) ? "ON" : "OFF",
    (green_state == GPIO_PIN_SET) ? "ON" : "OFF",
    (blue_state == GPIO_PIN_SET) ? "ON" : "OFF");

  if (len > 0) {
    CDC_Transmit_FS((uint8_t*)status_msg, len);
  }
}

/**
  * @brief  处理单色LED控制命令
  * @details 处理格式为"LED X"的命令，其中X为0或1
  *          这个命令专门控制红色LED，保持与原始版本的兼容性
  *
  * @param  cmd: 命令字符串指针
  * @param  len: 命令长度
  */
static void ProcessLEDCommand(uint8_t* cmd, uint16_t len)
{
  /* 提取LED状态参数 */
  uint8_t led_state = (cmd[LED_CMD_PREFIX_LEN] == '1') ? 1 : 0;

  /* 控制红色LED */
  SetSingleLED(LED_R_GPIO_Port, LED_R_Pin, led_state);

  /* 发送响应确认 */
  char response[32];
  int resp_len = snprintf(response, sizeof(response), "Red LED set to %d\r\n", led_state);
  if (resp_len > 0) {
    CDC_Transmit_FS((uint8_t*)response, resp_len);
  }
}

/**
  * @brief  处理RGB LED控制命令
  * @details 处理格式为"RGB R G B"的命令，其中R、G、B为0-255的数值
  *          这个命令允许独立控制RGB三个LED的亮度
  *
  *          注意：由于硬件限制，这里使用简单的开关控制
  *          数值 > 0 表示开启，= 0 表示关闭
  *
  * @param  cmd: 命令字符串指针
  * @param  len: 命令长度
  */
static void ProcessRGBCommand(uint8_t* cmd, uint16_t len)
{
  int r, g, b;

  /* 解析RGB参数 */
  int parsed = sscanf((char*)cmd + RGB_CMD_PREFIX_LEN, "%d %d %d", &r, &g, &b);

  /* 参数验证 */
  if (parsed != 3) {
    const char* error = "ERROR: Invalid RGB format. Use 'RGB R G B' (0-255 each)\r\n";
    CDC_Transmit_FS((uint8_t*)error, strlen(error));
    return;
  }

  /* 范围检查 */
  if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
    const char* error = "ERROR: RGB values must be between 0 and 255\r\n";
    CDC_Transmit_FS((uint8_t*)error, strlen(error));
    return;
  }

  /* 设置RGB LED状态（简单开关控制：>0为开，=0为关） */
  SetSingleLED(LED_R_GPIO_Port, LED_R_Pin, (r > 0) ? 1 : 0);
  SetSingleLED(LED_G_GPIO_Port, LED_G_Pin, (g > 0) ? 1 : 0);
  SetSingleLED(LED_B_GPIO_Port, LED_B_Pin, (b > 0) ? 1 : 0);

  /* 发送响应确认 */
  char response[64];
  int resp_len = snprintf(response, sizeof(response),
    "RGB LED set to [%d, %d, %d]\r\n", r, g, b);
  if (resp_len > 0) {
    CDC_Transmit_FS((uint8_t*)response, resp_len);
  }
}

/**
  * @brief  设置单个LED的状态
  * @details 这是一个通用的LED控制函数，可以控制任意连接到GPIO的LED
  *
  * @param  GPIO_Port: LED所在的GPIO端口（GPIO_TypeDef指针）
  * @param  GPIO_Pin:  LED所在的GPIO引脚
  * @param  state:     LED状态 (1=开启, 0=关闭)
  */
static void SetSingleLED(GPIO_TypeDef* GPIO_Port, uint16_t GPIO_Pin, uint8_t state)
{
  HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin,
                    (state) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
#endif  // !USE_USB_PORT_CLASS

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
