#ifndef PIN_MAP_H
#define PIN_MAP_H

/******************************************************************************
 *
 * @file       pin_map.hpp
 * @brief      RoboMaster开发板C型引脚映射头文件
 * @note
 * - 开发板主控: STM32F407IGH6
 * - 工作电压: 8V~28V
 * - 工作温度: 0~55°C
 * - 物理尺寸: 60×41×16.3 mm
 * @author     myself
 * @date       2025/11/15
 *
 *****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

/* ==================================================================================================
 */
/*                                                                                                    */
/*                                     外设功能定义 */
/*                                                                                                    */
/* ==================================================================================================
 */

/**
 * @defgroup LED_Definitions LED指示灯定义
 * @{
 *
 * @brief 开发板集成1颗共阳极RGB LED指示灯
 *
 * @note 控制逻辑:
 * - IO口输出高电平: 对应LED熄灭 (共阳极)
 * - IO口输出低电平: 对应LED点亮
 * - 可通过PWM控制亮度
 *
 * @warning 使用PWM控制时，频率建议1kHz以上，占空比0%~100%对应亮度0%~100%
 */
#define LED_R_PIN GPIO_PIN_12 /*!< 红色LED控制引脚 */
#define LED_R_GPIO_PORT GPIOH /*!< 红色LED所在GPIO端口 */
#define LED_G_PIN GPIO_PIN_11 /*!< 绿色LED控制引脚 */
#define LED_G_GPIO_PORT GPIOH /*!< 绿色LED所在GPIO端口 */
#define LED_B_PIN GPIO_PIN_10 /*!< 蓝色LED控制引脚 */
#define LED_B_GPIO_PORT GPIOH /*!< 蓝色LED所在GPIO端口 */

#define LED_RED_ON() HAL_GPIO_WritePin(LED_R_GPIO_PORT, LED_R_PIN, GPIO_PIN_RESET)
#define LED_RED_OFF() HAL_GPIO_WritePin(LED_R_GPIO_PORT, LED_R_PIN, GPIO_PIN_SET)
#define LED_GREEN_ON() HAL_GPIO_WritePin(LED_G_GPIO_PORT, LED_G_PIN, GPIO_PIN_RESET)
#define LED_GREEN_OFF() HAL_GPIO_WritePin(LED_G_GPIO_PORT, LED_G_PIN, GPIO_PIN_SET)
#define LED_BLUE_ON() HAL_GPIO_WritePin(LED_B_GPIO_PORT, LED_B_PIN, GPIO_PIN_RESET)
#define LED_BLUE_OFF() HAL_GPIO_WritePin(LED_B_GPIO_PORT, LED_B_PIN, GPIO_PIN_SET)
#define LED_ALL_OFF()    \
    do {                 \
        LED_RED_OFF();   \
        LED_GREEN_OFF(); \
        LED_BLUE_OFF();  \
    } while (0)
/**
 * @}
 */

/**
 * @defgroup Power_Control 电源控制
 * @{
 *
 * @brief 5V激光接口控制
 *
 * @note
 * - 用于控制5V激光器电源(红点激光器)
 * - 通过PC8引脚控制MOS管开关
 * - 可通过PWM控制激光器亮度
 * - 最大输出电流: 5A (7路PWM接口总和)
 *
 * @warning
 * - 该引脚控制的是VCC_5V_M电源网络，最大可提供5A电流
 * - 不要超过电源负载能力，避免过热
 */
#define LASER_CTRL_PIN GPIO_PIN_8        /*!< 激光器控制引脚 */
#define LASER_CTRL_GPIO_PORT GPIOC       /*!< 激光器控制所在GPIO端口 */
#define LASER_CTRL_TIM TIM3              /*!< 控制定时器 */
#define LASER_CTRL_CHANNEL TIM_CHANNEL_3 /*!< PWM通道3 */

/**
 * @brief 电压检测引脚
 *
 * @note
 * - 用于检测输入电压VCC_BAT
 * - 分压后连接到ADC (PF10)
 * - D10起到箝压保护作用
 * - 计算公式: Vbat = ADC_value * 3.3 / 4095 * (220K + 22K) / 22K
 *
 * @warning
 * - 该ADC通道使用12位精度
 * - 输入电压范围0-28V，分压后不超过3.3V
 */
#define VOLTAGE_SENSE_PIN GPIO_PIN_10           /*!< 电压检测引脚 */
#define VOLTAGE_SENSE_GPIO_PORT GPIOF           /*!< 电压检测所在GPIO端口 */
#define VOLTAGE_SENSE_ADC_CHANNEL ADC_CHANNEL_8 /*!< ADC通道8 */
/**
 * @}
 */

/**
 * @defgroup USB_Interface USB接口定义
 * @{
 *
 * @brief USB全速接口(12Mbps)
 *
 * @note
 * - 符合USB2.0协议规范
 * - 从机模式仅支持全速(12Mbps)
 * - 供电能力有限，仅能驱动STM32及部分板载外设
 * - 不能驱动由VCC_5V_M供电的外设(如PWM接口设备)
 *
 * @warning
 * - USB供电时请勿连接大功率外设
 * - 长时间大电流工作可能导致USB接口过热
 */
#define USB_DM_PIN GPIO_PIN_11 /*!< USB数据负 */
#define USB_DM_GPIO_PORT GPIOA
#define USB_DP_PIN GPIO_PIN_12 /*!< USB数据正 */
#define USB_DP_GPIO_PORT GPIOA
#define USB_OTG_PIN GPIO_PIN_10 /*!< USB OTG检测 */
#define USB_OTG_GPIO_PORT GPIOA
/**
 * @}
 */

/**
 * @defgroup Buttons 按键定义
 * @{
 *
 * @brief 开发板集成两个按键
 *
 * @note
 * - 复位按键(RST): 硬件复位，无需软件控制
 * - 用户按键(KEY): 按下时PA0为低电平
 * - 内部已配置上拉电阻
 *
 * @warning
 * - 用户按键使用内部上拉，外部不需要额外上拉电阻
 */
#define USER_KEY_PIN GPIO_PIN_0  /*!< 用户按键引脚 */
#define USER_KEY_GPIO_PORT GPIOA /*!< 用户按键所在GPIO端口 */
#define USER_KEY_ACTIVE_LOW      /*!< 按键按下时为低电平 */
/**
 * @}
 */

/**
 * @defgroup Configurable_IO 可配置I/O接口
 * @{
 *
 * @brief 8-pin牛角座，支持I2C或SPI设备
 *
 * @note
 * - 支持3.3V或5V通信电平
 * - 使用5V外设时需手动焊接R210电阻并去除R209
 * - I2C2: PF0(SDA), PF1(SCL)
 * - SPI2: PB12(CS), PB13(CLK), PB14(MISO), PB15(MOSI)
 *
 * @warning
 * - 同时只能使用I2C或SPI中的一种模式
 * - 5V模式下确保外设兼容5V电平
 */
/* I2C2接口 */
#define I2C2_SCL_PIN GPIO_PIN_1 /*!< I2C2时钟线 */
#define I2C2_SCL_GPIO_PORT GPIOF
#define I2C2_SDA_PIN GPIO_PIN_0 /*!< I2C2数据线 */
#define I2C2_SDA_GPIO_PORT GPIOF
#define I2C2_SPEED 400000 /*!< 400kHz标准速率 */

/* SPI2接口 */
#define SPI2_CS_PIN GPIO_PIN_12 /*!< SPI2片选 */
#define SPI2_CS_GPIO_PORT GPIOB
#define SPI2_CLK_PIN GPIO_PIN_13 /*!< SPI2时钟 */
#define SPI2_CLK_GPIO_PORT GPIOB
#define SPI2_MISO_PIN GPIO_PIN_14 /*!< SPI2主入从出 */
#define SPI2_MISO_GPIO_PORT GPIOB
#define SPI2_MOSI_PIN GPIO_PIN_15 /*!< SPI2主出从入 */
#define SPI2_MOSI_GPIO_PORT GPIOB
#define SPI2_SPEED 10000000 /*!< 10MHz最大速率 */
/**
 * @}
 */

/**
 * @defgroup UART_Interface UART接口定义
 * @{
 *
 * @brief 开发板集成2路UART接口
 *
 * @note
 * - UART1(4-pin): 对应STM32的UART6，外壳丝印标注为UART2
 * - UART6(3-pin): 对应STM32的UART1，外壳丝印标注为UART1
 * - 仅支持3.3V和5V电平
 * - 与RS485/RS232通信需外置电平转换芯片
 *
 * @warning
 * - 接口丝印与STM32实际串口配置不对应！
 *   外壳丝印UART1 -> STM32 UART6
 *   外壳丝印UART2 -> STM32 UART1
 * - UART6(3-pin)线序与裁判系统电源模块一致，通信时需交叉TX/RX
 */
/* UART1 (外壳丝印UART2) - 4-pin接口 */
#define UART1_TX_PIN GPIO_PIN_9 /*!< UART1发送引脚 */
#define UART1_TX_GPIO_PORT GPIOA
#define UART1_RX_PIN GPIO_PIN_7 /*!< UART1接收引脚 */
#define UART1_RX_GPIO_PORT GPIOB
#define UART1_BAUDRATE 115200 /*!< 默认波特率 */
#define UART1_INSTANCE USART1 /*!< UART1实例 */

/* UART6 (外壳丝印UART1) - 3-pin接口 */
#define UART6_TX_PIN GPIO_PIN_14 /*!< UART6发送引脚 */
#define UART6_TX_GPIO_PORT GPIOG
#define UART6_RX_PIN GPIO_PIN_9 /*!< UART6接收引脚 */
#define UART6_RX_GPIO_PORT GPIOG
#define UART6_BAUDRATE 115200 /*!< 默认波特率 */
#define UART6_INSTANCE USART6 /*!< UART6实例 */
/**
 * @}
 */

/**
 * @defgroup CAN_Interface CAN总线接口定义
 * @{
 *
 * @brief 开发板集成2路CAN总线接口
 *
 * @note
 * - CAN1: 2-pin接口，最大速率1Mbps
 * - CAN2: 4-pin接口，最大速率1Mbps
 * - 用于控制RoboMaster电调或与其他设备通信
 * - 使用TJA1044收发器
 *
 * @warning
 * - 确保终端电阻正确配置(120Ω)
 * - 长距离通信时建议使用屏蔽双绞线
 */
/* CAN1接口 (2-pin) */
#define CAN1_TX_PIN GPIO_PIN_1 /*!< CAN1发送引脚 */
#define CAN1_TX_GPIO_PORT GPIOD
#define CAN1_RX_PIN GPIO_PIN_0 /*!< CAN1接收引脚 */
#define CAN1_RX_GPIO_PORT GPIOD
#define CAN1_SPEED 1000000 /*!< 1Mbps */
#define CAN1_INSTANCE CAN1 /*!< CAN1实例 */

/* CAN2接口 (4-pin) */
#define CAN2_TX_PIN GPIO_PIN_6 /*!< CAN2发送引脚 */
#define CAN2_TX_GPIO_PORT GPIOB
#define CAN2_RX_PIN GPIO_PIN_5 /*!< CAN2接收引脚 */
#define CAN2_RX_GPIO_PORT GPIOB
#define CAN2_SPEED 1000000 /*!< 1Mbps */
#define CAN2_INSTANCE CAN2 /*!< CAN2实例 */
/**
 * @}
 */

/**
 * @defgroup PWM_Interface PWM接口定义
 * @{
 *
 * @brief 7路PWM输出接口，用于连接5V舵机或其他PWM驱动模块
 *
 * @note
 * - 总输出电流最大可达5A
 * - 频率范围: 50Hz~100kHz (舵机推荐50Hz)
 * - 占空比范围: 0%~100%
 * - 支持TIM1(4路)和TIM8(3路)
 *
 * @warning
 * - 不要超过5A总电流限制
 * - 大电流负载时注意散热
 */
#define PWM1_PIN GPIO_PIN_9 /*!< PWM1 (TIM1_CH1) */
#define PWM1_GPIO_PORT GPIOE
#define PWM1_TIMER TIM1
#define PWM1_CHANNEL TIM_CHANNEL_1

#define PWM2_PIN GPIO_PIN_11 /*!< PWM2 (TIM1_CH2) */
#define PWM2_GPIO_PORT GPIOE
#define PWM2_TIMER TIM1
#define PWM2_CHANNEL TIM_CHANNEL_2

#define PWM3_PIN GPIO_PIN_13 /*!< PWM3 (TIM1_CH3) */
#define PWM3_GPIO_PORT GPIOE
#define PWM3_TIMER TIM1
#define PWM3_CHANNEL TIM_CHANNEL_3

#define PWM4_PIN GPIO_PIN_14 /*!< PWM4 (TIM1_CH4) */
#define PWM4_GPIO_PORT GPIOE
#define PWM4_TIMER TIM1
#define PWM4_CHANNEL TIM_CHANNEL_4

#define PWM5_PIN GPIO_PIN_6 /*!< PWM5 (TIM8_CH1) */
#define PWM5_GPIO_PORT GPIOC
#define PWM5_TIMER TIM8
#define PWM5_CHANNEL TIM_CHANNEL_1

#define PWM6_PIN GPIO_PIN_6 /*!< PWM6 (TIM8_CH2) */
#define PWM6_GPIO_PORT GPIOI
#define PWM6_TIMER TIM8
#define PWM6_CHANNEL TIM_CHANNEL_2

#define PWM7_PIN GPIO_PIN_7 /*!< PWM7 (TIM8_CH3) */
#define PWM7_GPIO_PORT GPIOI
#define PWM7_TIMER TIM8
#define PWM7_CHANNEL TIM_CHANNEL_3
/**
 * @}
 */

/**
 * @defgroup DBUS_Interface DBUS接口定义
 * @{
 *
 * @brief DJI遥控器通用协议接口
 *
 * @note
 * - 与PWM接口共用24-pin连接器
 * - 信号经反相电路后连接到UART3_RX
 * - 波特率一般设置为100kbps
 * - 位置: Pin C8 (24-pin连接器)
 *
 * @warning
 * - 必须设置正确的波特率(100kbps)
 * - 需要配置UART3为接收模式
 */
#define DBUS_RX_PIN GPIO_PIN_11 /*!< DBUS接收引脚 */
#define DBUS_RX_GPIO_PORT GPIOC
#define DBUS_INSTANCE USART3 /*!< DBUS使用的UART实例 */
#define DBUS_BAUDRATE 100000 /*!< 100kbps */
/**
 * @}
 */

/**
 * @defgroup Camera_Interface 摄像头接口定义
 * @{
 *
 * @brief 18-pin FPC接口，支持DCMI数字摄像头
 *
 * @note
 * - 支持8位CMOS照相机模块
 * - 支持多种数据格式(RGB565, YUV, JPEG等)
 * - 最大分辨率: 1600×1200 (200万像素)
 * - 像素时钟最大54MHz
 * - I2C1用于摄像头配置
 *
 * @warning
 * - 摄像头初始化时序很重要，参考OV2640/OV7725等模块手册
 * - 确保DMA配置正确，避免数据丢失
 */
/* DCMI接口 */
#define DCMI_D0_PIN GPIO_PIN_9 /*!< 数据线0 */
#define DCMI_D0_GPIO_PORT GPIOH
#define DCMI_D1_PIN GPIO_PIN_7 /*!< 数据线1 */
#define DCMI_D1_GPIO_PORT GPIOC
#define DCMI_D2_PIN GPIO_PIN_0 /*!< 数据线2 */
#define DCMI_D2_GPIO_PORT GPIOE
#define DCMI_D3_PIN GPIO_PIN_1 /*!< 数据线3 */
#define DCMI_D3_GPIO_PORT GPIOE
#define DCMI_D4_PIN GPIO_PIN_4 /*!< 数据线4 */
#define DCMI_D4_GPIO_PORT GPIOE
#define DCMI_D5_PIN GPIO_PIN_4 /*!< 数据线5 */
#define DCMI_D5_GPIO_PORT GPIOI
#define DCMI_D6_PIN GPIO_PIN_5 /*!< 数据线6 */
#define DCMI_D6_GPIO_PORT GPIOE
#define DCMI_D7_PIN GPIO_PIN_6 /*!< 数据线7 */
#define DCMI_D7_GPIO_PORT GPIOE

#define DCMI_PCLK_PIN GPIO_PIN_6 /*!< 像素时钟 */
#define DCMI_PCLK_GPIO_PORT GPIOA
#define DCMI_HSYNC_PIN GPIO_PIN_8 /*!< 行同步信号 */
#define DCMI_HSYNC_GPIO_PORT GPIOH
#define DCMI_VSYNC_PIN GPIO_PIN_5 /*!< 帧同步信号 */
#define DCMI_VSYNC_GPIO_PORT GPIOI

/* I2C1 (摄像头配置) */
#define CAMERA_I2C_SDA_PIN GPIO_PIN_9 /*!< I2C1数据线 */
#define CAMERA_I2C_SDA_GPIO_PORT GPIOB
#define CAMERA_I2C_SCL_PIN GPIO_PIN_8 /*!< I2C1时钟线 */
#define CAMERA_I2C_SCL_GPIO_PORT GPIOB
#define CAMERA_I2C_SPEED 400000 /*!< 400kHz */
/**
 * @}
 */

/**
 * @defgroup Buzzer_Interface 蜂鸣器接口定义
 * @{
 *
 * @brief 贴片式无源蜂鸣器
 *
 * @note
 * - 额定频率: 4000Hz
 * - 通过PWM驱动
 * - 可通过调节PWM频率改变音调
 * - 最大驱动电流: 30mA
 *
 * @warning
 * - 无源蜂鸣器需要特定频率驱动
 * - 频率过低或过高可能无声
 */
#define BUZZER_PIN GPIO_PIN_14 /*!< 蜂鸣器控制引脚 */
#define BUZZER_GPIO_PORT GPIOD
#define BUZZER_TIMER TIM4
#define BUZZER_CHANNEL TIM_CHANNEL_3
#define BUZZER_DEFAULT_FREQ 4000 /*!< 默认频率4kHz */
/**
 * @}
 */

/**
 * @defgroup IMU_Interface 六轴IMU接口定义
 * @{
 *
 * @brief BMI088六轴惯性测量单元
 *
 * @note
 * - 3轴加速度计 + 3轴陀螺仪
 * - SPI通信，最大10MHz
 * - 内置加热电路，改善温飘问题
 * - 加热功率: 0.58W (TIM10_CH1高电平)
 * - 推荐加热温度: 比环境高15~20℃
 *
 * @warning
 * - 加热电路使用5V电源，长时间加热注意散热
 * - 加热温度过高可能损坏传感器
 */
/* SPI1接口 */
#define IMU_SPI_CS_ACCEL_PIN GPIO_PIN_4 /*!< 加速度计片选 */
#define IMU_SPI_CS_ACCEL_PORT GPIOA
#define IMU_SPI_CS_GYRO_PIN GPIO_PIN_0 /*!< 陀螺仪片选 */
#define IMU_SPI_CS_GYRO_PORT GPIOB
#define IMU_SPI_SCK_PIN GPIO_PIN_3 /*!< SPI时钟 */
#define IMU_SPI_SCK_PORT GPIOB
#define IMU_SPI_MOSI_PIN GPIO_PIN_7 /*!< SPI MOSI */
#define IMU_SPI_MOSI_PORT GPIOA
#define IMU_SPI_MISO_PIN GPIO_PIN_4 /*!< SPI MISO */
#define IMU_SPI_MISO_PORT GPIOB

/* 中断引脚 */
#define IMU_INT1_ACCEL_PIN GPIO_PIN_4 /*!< 加速度计中断1 */
#define IMU_INT1_ACCEL_PORT GPIOC
#define IMU_INT1_GYRO_PIN GPIO_PIN_5 /*!< 陀螺仪中断1 */
#define IMU_INT1_GYRO_PORT GPIOC

/* 加热控制 */
#define IMU_HEATER_PIN GPIO_PIN_6 /*!< 加热控制引脚 */
#define IMU_HEATER_PORT GPIOF
#define IMU_HEATER_TIMER TIM10
#define IMU_HEATER_CHANNEL TIM_CHANNEL_1
/**
 * @}
 */

/**
 * @defgroup Magnetometer_Interface 磁力计接口定义
 * @{
 *
 * @brief IST8310三轴磁力计
 *
 * @note
 * - I2C通信，最大400kHz
 * - 默认I2C地址: 0x0E
 * - DRDY引脚用于数据就绪中断
 * - RSTN引脚用于复位控制
 *
 * @warning
 * - 磁力计附近2mm内不要布其他器件
 * - 远离功率线和电机，避免磁场干扰
 */
#define MAG_I2C_SDA_PIN GPIO_PIN_9 /*!< I2C3数据线 */
#define MAG_I2C_SDA_PORT GPIOC
#define MAG_I2C_SCL_PIN GPIO_PIN_8 /*!< I2C3时钟线 */
#define MAG_I2C_SCL_PORT GPIOA
#define MAG_I2C_SPEED 400000 /*!< 400kHz */

#define MAG_DRDY_PIN GPIO_PIN_3 /*!< 数据就绪引脚 */
#define MAG_DRDY_PORT GPIOG
#define MAG_RSTN_PIN GPIO_PIN_6 /*!< 复位控制引脚 */
#define MAG_RSTN_PORT GPIOG
#define MAG_I2C_ADDR 0x0E /*!< 默认I2C地址 */
/**
 * @}
 */

/* ==================================================================================================
 */
/*                                                                                                    */
/*                                     物理接口线序定义 */
/*                                                                                                    */
/* ==================================================================================================
 */

/**
 * @defgroup Connector_Pinouts 物理接口线序
 * @{
 *
 * @brief 各物理接口的引脚排列顺序
 *
 * @note 这些定义用于连接器线序，实际使用时需按照此顺序连接
 */

/** UART1 (4-pin) 接口线序 (外壳丝印UART2) */
#define UART1_CONNECTOR_PIN1 "RXD" /*!< 1: RXD */
#define UART1_CONNECTOR_PIN2 "TXD" /*!< 2: TXD */
#define UART1_CONNECTOR_PIN3 "GND" /*!< 3: GND */
#define UART1_CONNECTOR_PIN4 "5V"  /*!< 4: 5V */

/** UART6 (3-pin) 接口线序 (外壳丝印UART1) */
#define UART6_CONNECTOR_PIN1 "GND" /*!< 1: GND */
#define UART6_CONNECTOR_PIN2 "TXD" /*!< 2: TXD */
#define UART6_CONNECTOR_PIN3 "RXD" /*!< 3: RXD */

/** CAN1 (2-pin) 接口线序 */
#define CAN1_CONNECTOR_PIN1 "CANL" /*!< 1: CANL (黑色) */
#define CAN1_CONNECTOR_PIN2 "CANH" /*!< 2: CANH (红色) */

/** CAN2 (4-pin) 接口线序 */
#define CAN2_CONNECTOR_PIN1 "5V"   /*!< 1: 5V (红色) */
#define CAN2_CONNECTOR_PIN2 "GND"  /*!< 2: GND (灰色) */
#define CAN2_CONNECTOR_PIN3 "CANH" /*!< 3: CANH (灰色) */
#define CAN2_CONNECTOR_PIN4 "CANL" /*!< 4: CANL (灰色) */

/** 24-pin PWM/DBUS连接器关键引脚 */
#define PWM_CONNECTOR_PIN_A1 "PGND" /*!< A1: 电源地 */
#define PWM_CONNECTOR_PIN_B1 "5V_M" /*!< B1: 5V主电源 */
#define PWM_CONNECTOR_PIN_A8 "PGND" /*!< A8: 电源地 */
#define PWM_CONNECTOR_PIN_B8 "5V"   /*!< B8: 5V辅助电源 */
#define PWM_CONNECTOR_PIN_C8 "DBUS" /*!< C8: DBUS信号 */

/** 数字摄像头FPC接口 (18-pin) 关键引脚 */
#define CAMERA_FPC_PIN1 "I2C1_SCL"   /*!< 1: I2C1时钟 */
#define CAMERA_FPC_PIN2 "I2C1_SDA"   /*!< 2: I2C1数据 */
#define CAMERA_FPC_PIN3 "PCLK_OUT"   /*!< 3: 像素时钟 */
#define CAMERA_FPC_PIN4 "DCMI_HREF"  /*!< 4: 行有效信号 */
#define CAMERA_FPC_PIN5 "DCMI_VSYNC" /*!< 5: 帧同步信号 */
/**
 * @}
 */

/**
 * @brief 24-pin PWM/DBUS连接器完整引脚定义
 *
 * @note 引脚排列 (3行×8列):
 *        A1 A2 A3 A4 A5 A6 A7 A8
 *        B1 B2 B3 B4 B5 B6 B7 B8
 *        C1 C2 C3 C4 C5 C6 C7 C8
 *
 * @warning 重要引脚:
 * - A1, A5-A8, B6-B8, C1-C7: 电源地(PGND)
 * - B1: 5V_M (主5V电源，最大5A)
 * - B2: TIM1_CH2 (PWM2)
 * - B3: TIM1_CH4 (PWM4)
 * - B4: TIM8_CH2 (PWM6)
 * - B5: TIM8_CH1 (PWM5)
 * - B8: 5V (辅助5V电源)
 * - C2: TIM1_CH1 (PWM1)
 * - C3: TIM1_CH3 (PWM3)
 * - C4: TIM8_CH3 (PWM7)
 * - C8: DBUS (遥控器信号)
 */
#define PWM_CONNECTOR_LAYOUT                                                              \
    "A1:PGND  A2:TIM1_CH2 A3:TIM1_CH4 A4:TIM8_CH2 A5:PGND  A6:PGND  A7:PGND  A8:DBUS\n"   \
    "B1:5V_M   B2:TIM1_CH1 B3:TIM1_CH3 B4:TIM8_CH1 B5:TIM8_CH3 B6:PGND  B7:PGND  B8:5V\n" \
    "C1:PGND  C2:PGND     C3:PGND     C4:PGND     C5:PGND  C6:PGND  C7:PGND  C8:PGND"

// /*
// ==================================================================================================
// */
// /* */
// /*                                     实用宏定义和函数原型 */
// /* */
// /*
// ==================================================================================================
// */

// /**
//  * @brief 初始化所有板载外设
//  *
//  * @note 此函数会初始化:
//  * - 时钟系统
//  * - GPIO
//  * - LED
//  * - 按键
//  * - 各通信接口
//  * - 传感器
//  *
//  * @warning 在调用其他外设函数前必须先调用此函数
//  */
// void BSP_Board_Init(void);

// /**
//  * @brief 获取板载电压(mV)
//  *
//  * @return uint16_t 电池电压(mV)
//  *
//  * @note
//  * - 使用ADC读取PF10引脚
//  * - 内部已做滤波处理
//  * - 返回值为mV单位
//  */
// uint16_t BSP_Get_Battery_Voltage(void);

// /**
//  * @brief 设置LED颜色
//  *
//  * @param red 红色分量(0-255)
//  * @param green 绿色分量(0-255)
//  * @param blue 蓝色分量(0-255)
//  *
//  * @note 使用PWM控制实现RGB混合
//  */
// void BSP_Set_LED_Color(uint8_t red, uint8_t green, uint8_t blue);

// /**
//  * @brief 检查用户按键状态
//  *
//  * @return uint8_t 1=按下, 0=释放
//  */
// uint8_t BSP_Check_User_Key(void);

#ifdef __cplusplus
}
#endif

#endif  // PIN_MAP_H

/**
 * @page hardware_notes 硬件注意事项
 *
 * @section power_notes 电源注意事项
 * - 开发板支持8-28V输入，推荐24V
 * - 5V_M电源网络最大输出5A，用于PWM接口
 * - 5V电源网络最大输出1A，用于板载器件
 * - USB供电仅支持STM32和部分外设，不能驱动PWM设备
 *
 * @section communication_notes 通信注意事项
 * - CAN总线终端电阻必须配置(120Ω)
 * - UART6(3-pin)与裁判系统通信时需要交叉TX/RX
 * - 摄像头接口信号敏感，避免长距离走线
 *
 * @section sensor_notes 传感器注意事项
 * - IMU加热温度不要超过60℃
 * - 磁力计远离电机和电源线
 * - 摄像头工作时会产生热量，注意散热
 *
 * @section debug_notes 调试注意事项
 * - SWD接口支持J-Link/ST-Link
 * - DFU模式需要配置BOOT0=1, BOOT1=0
 * - 调试时建议使用外部电源，避免USB供电不足
 */
