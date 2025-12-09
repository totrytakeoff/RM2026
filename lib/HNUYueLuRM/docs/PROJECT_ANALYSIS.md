项目整体结构与文件/模块功能说明
=================================

说明：本文件为仓库 `basic_framework-master` 的结构与模块级分析（中文），帮助快速定位各文件职责与关系。基于已有代码与命名约定进行说明；若需我为每个单文件生成逐行注释或 CSV 列表，请回复确认。 

**总体分层**
- **底层（Drivers & BSP）**: 与硬件、HAL、CMSIS、外设驱动密切相关，封装 MCU 外设接口（GPIO、TIM、USART、SPI、I2C、CAN、ADC、DAC、RTC、USB 等），为上层提供统一抽象。
- **中间层（Middlewares & modules）**: 包含 RTOS（FreeRTOS）、USB 库（STM32 USB Device）、SEGGER RTT、CMSIS-DSP 等第三方中间件；modules 目录包含功能模块（电机驱动、IMU、遥控、裁判系统、超级电容等）和算法实现（EKF、卡尔曼等）。
- **上层（application）**: 机器人应用（chassis、gimbal、shoot、cmd、robot 等），由 module 提供能力，形成完整机器人控制逻辑。

**根目录关键文件说明**
- `README.md`：项目总览、使用说明、架构与开发指南入口。
- `CMakeLists.txt`：CMake 构建配置（若偏好现代构建可用），设置工具链 `arm-none-eabi-*`、链接 CMSIS-DSP 库、收集源文件并生成 elf/hex/bin。
- `Makefile`：默认 Make 构建脚本（generated），列出了大部分 `Src`、`Drivers`、`Middlewares`、`bsp`、`modules`、`application` 源文件；包含 `download_dap`、`download_jlink` 任务用于烧写。
- `startup_stm32f407xx.s`：启动汇编，初始化向量表与初始堆栈指针。
- `STM32F407IGHx_FLASH.ld`：链接脚本，定义 flash/ram 区域与段放置地址。
- `openocd_dap.cfg` / `openocd_jlink.cfg` / `stm32.jflash`：调试与烧录配置。
- `.Doc/`：一系列开发文档（架构、VSCode+Ozone 指南、PID 调参、如何定位 bug 等），是快速上手的首选阅读材料。

**`Inc/`（公共头文件）**
- `main.h`：board pin 定义与应用中使用的全局宏（LED、Buzzer、外设引脚等）。
- `*.h`（`adc.h`,`can.h`,`i2c.h`,`spi.h`,`tim.h`,`usart.h`,`usb_device.h`,`usbd_*` 等）：外设接口声明，由 `Src/` 中的 `bsp` 或 `Src` 实现文件配合 `Drivers` HAL 使用。
- `FreeRTOSConfig.h`：FreeRTOS 配置宏。

**`Src/`（工程源码 — 系统/外设 glue）**
（列出并说明常见文件，实际项目内有很多 HAL 源来自 `Drivers`）
- `main.c`：程序入口；调用 `HAL_Init()` -> 时钟配置 -> 初始化各外设（通过 MX_*_Init） -> 调用 `RobotInit()` -> 启动 FreeRTOS 调度。系统启动和全局初始化核心。
- `system_stm32f4xx.c`：系统时钟、向量与系统级设置实现（由 CubeMX 生成）。
- `stm32f4xx_it.c`：中断处理入口；IRQ handler 的中断回调分发。
- `stm32f4xx_hal_msp.c`：HAL 底层 MCU 外设 MSP 初始化（时钟、GPIO、NVIC 配置）
- `usbd_*`, `usb_device.c`：USB device 层实现（CDC 类用作 VCP 终端）。
- `rtc.c`：RTC 初始化与 MSP 支持（使用 HSE_DIV30 作为 RTC 时钟源，设置同步/异步分频）；负责 RTC 外设的 HAL 封装。
- `tim.c / adc.c / can.c / i2c.c / spi.c / usart.c / dma.c / gpio.c / etc.`：对 STM32 HAL 的封装初始化函数（MX_*_Init 在这些文件中），由 CubeMX 生成，作为板级外设初始化点。
- `freertos.c`：FreeRTOS 的初始化（任务、队列、定时器等创建）接口 `MX_FREERTOS_Init()`。

**`bsp/`（Board Support Package — 板级封装）**
作用：将底层 HAL/外设初始化封装为更易用的接口，并提供实例化模式（类似于面向对象的 C 风格）。

- `bsp_init.h/.c`：统一 BSP 初始化入口（`BSPInit()`，会做 DWT、日志等基础初始化），由 `RobotInit()` 调用。
- `bsp_tools.c`：常用工具函数，任务/定时/调度工具等。
- `dwt/`：DWT 时间/延时支持，提供高精度计时（用于 imu 采样以保证实时性）。
- `adc/`, `can/`, `iic/`, `spi/`, `usart/`, `usb/`, `pwm/`, `flash/`, `gpio/` 等子目录：每个子目录提供对应外设的封装（`bsp_xxx.c/.h`），实现设备驱动模式、回调与实例注册。
- `log/`：日志输出（可通过 RTT / 串口输出），封装 `LOGINFO/LOGWARN/...` 宏。

**`Drivers/`（ST 官方库与 CMSIS）**
- `CMSIS/`：CPU 与 MCU 抽象层头文件（`core_cm4.h`, `stm32f4xx.h` 等）。
- `STM32F4xx_HAL_Driver/`：ST HAL 的源代码（大部分 MCU 外设实现都在此处），项目直接引用这些文件以编译 HAL 实现。

**`Middlewares/`（第三方中间件）**
- `FreeRTOS/`：FreeRTOS 实现（任务、队列、内存管理、移植层 `port.c` 等）。
- `SEGGER/RTT`：SEGGER RTT 用于实时打印日志与 Ozone 支持（可以替代串口作为调试输出）。
- `STM32_USB_Device_Library/`：STM 的 USB device 库（CDC、core 层实现）。
- `ARM/DSP`：CMSIS-DSP 库的头与二进制库（`libCMSISDSP.a`），用于高性能数学/滤波/矩阵运算。

**`modules/`（功能模块）**
总体：模块化设计，分为硬件驱动模块（电机、IMU、传感器）、通信模块（CAN、master-machine、unicomm）、功能模块（OLED、buzzer、super_cap、TFminiPlus）、算法模块（滤波、EKF、CRC）及消息中心等。

- `motor/`：包含不同厂商/类型电机驱动实现
  - `DJImotor/`：DJI 电机驱动协议实现（`dji_motor.c/.h`）
  - `HTmotor/`：海泰/HT04 电机驱动实现
  - `LKmotor/`：LK 系列电机驱动（LK9025）
  - `DMmotor/`、`step_motor/`、`servo_motor/`：其它电机/步进/舵机驱动
  - `motor_task.c`：集中调度各类型电机控制调用（`MotorControlTask()`）。

- `imu/` 与 `BMI088/`：BMI088 传感器驱动、中间件、EKF/姿态初始化（`ins_task.c` 实现 1kHz 采样 + EKF 更新与温控 PWM 控制）。

- `ist8310/`：磁力计驱动（`ist8310.c/.h`）。

- `algorithm/`：包含 `QuaternionEKF.c`、`kalman_filter.c`、`controller.c`、CRC 实现等算法模块，为上层提供姿态解算、滤波、PID 算法。

- `referee/`：裁判系统数据解析与任务（`referee_task.c`、`rm_referee.c` 等），用于比赛裁判系统信息解析与 UI

- `can_comm/`：板间 CAN 通信封装，含报文打包/解析逻辑

- `message_center/`：模块间消息中心（pub-sub 或通知中心），用于模块解耦与消息转发

- `daemon/`：守护任务（看门狗式监控），用于检测模块离线/异常

（每个模块目录通常包含 `.md` 文档，说明如何初始化、注册实例与使用）

**`application/`（机器人具体应用）**
- `robot.c / robot.h / robot_def.h / robot_task.h`：机器人总体初始化入口（`RobotInit()`、`RobotTask()`），通过宏 `ONE_BOARD`/`GIMBAL_BOARD`/`CHASSIS_BOARD` 选择编译那些子系统。
- `cmd/robot_cmd.*`：命令解析层，负责接收上位机/遥控指令并下发到各模块。
- `gimbal/`：云台（gimbal）控制相关的初始化与任务，包含云台的 PID 与控制闭环。
- `chassis/`：底盘控制代码（含 mecanum / steering / balance 等实现），实现运动学/动力学转换并调用电机模块。
- `shoot/`：发射机构控制（拨盘、弹舱、触发等）。

**调试、构建与烧录**
- `.vscode/launch.json`、`tasks.json`：提供 VSCode 调试/编译任务（jlink/openocd/jflash 等）
- `Makefile` & `CMakeLists.txt`：两套构建脚本，`Makefile` 常用（列出所有源文件），`CMakeLists.txt` 为可选现代构建方式（支持 Ninja）。
- `openocd_*` / `JFlash`：烧写与在线调试命令样例在 Makefile 中已有 `download_dap` 与 `download_jlink`。

**建议的阅读顺序（快速上手）**
1. 阅读 `.Doc/架构介绍与开发指南.md` 与 `README.md`（把整体脉络搞清楚）。
2. 看 `application/robot_def.h` 确认当前板卡与编译宏设置（`ONE_BOARD` / `GIMBAL_BOARD` 等）。
3. 阅读 `main.c` 与 `BSPInit()`（`bsp/bsp_init.h`）确认初始化顺序与必须的 BSP。
4. 阅读 `modules` 中你要关注的模块（如 `modules/imu/ins_task.c`，`modules/motor/*`），并查看对应的 `bsp/*` 驱动。
5. 查看 `freertos.c` 与 `application/robot_task.h` 了解任务分配与优先级。

**如何获得“每个文件逐项”完整清单**
我可以为你自动生成一份完整的“文件路径 — 单行说明”清单（CSV/Markdown），或对关键文件（比如 `Src/*.c`, `modules/*`, `application/*`）做逐文件深入阅读并提取函数/接口说明。你想要哪种：
- 选项 A：生成完整 CSV（每行：路径, 简短功能说明）
- 选项 B：按文件夹分块，生成更详尽的 Markdown（每文件 3-6 行说明，并摘录关键函数签名）
- 选项 C：人工逐个文件添加注释（较慢），我会修改源文件并插入注释（需你确认是否允许改动源码）

我现在已经完成项目概要与关键文件的读取（`main.c`, `rtc.c`, `ins_task.c`, `motor_task.c`, `robot.c` 等）。下一步我可以：
- 将当前概要保存为 `PROJECT_ANALYSIS.md`（已创建）。
- 若你需要，我会继续自动生成“每文件清单”（选项 A 或 B）。

请回复你希望的下一步：生成 CSV 清单（A）、生成更详尽 Markdown（B）、或逐文件注释（C）。

---

（文件自动生成：PROJECT_ANALYSIS.md）
