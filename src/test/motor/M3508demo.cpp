//
// C++ 入口: M3508 电机控制测试
// 注意: 由于 HAL 与外设初始化为 C 文件实现，需要正确处理 extern "C"
//
// ========== 重要说明 ==========
// 1. 速度控制问题：
//    - M3508 速度环 PID 推荐参数：Kp=5~10, Ki=0.1~0.5, Kd=0
//    - 如果 Kp 过大（如15.0），会导致震荡和超调
//    - 如果 Kp 过小（如3.0），会导致启动力矩不足，电机不转
//    - 原因：Kp=3时，1000rpm误差产生3000电流(约3.66A)，无法克服静摩擦
//    - 已修正为 Kp=8.0, Ki=0.3（提供约9.76A启动电流）
//
// 2. 位置控制问题：
//    - 测试6使用的是简化P控制，不是完整PID
//    - 位置环：位置误差 × Kp = 目标速度
//    - 速度环：速度误差 × Kp = 输出电流
//    - 已添加死区判断（±5°）避免在目标位置震荡
//    - 推荐参数：位置环Kp=0.3~1.0, 速度环Kp=5~10
//
// 3. 控制模式选择：
//    - 测试1-2：使用完整PID的速度环控制（推荐）
//    - 测试3-4：使用完整PID的位置环控制（推荐）
//    - 测试5-6：简化P控制（快速测试用）
//

extern "C" {
#include "stm32f4xx_hal.h"

// 注意 硬件初始化文件一定要 extern "C"
#include "hal/can.h"
#include "hal/gpio.h"
// 来自 C 源文件的函数原型（C 链接）
void SystemClock_Config(void);
void Error_Handler(void);
void MX_CAN1_Init(void);
void MX_CAN2_Init(void);
void can_filter_init(void);
void MX_GPIO_Init(void);
}

#include "drivers/motor/gm6020.hpp"
#include "drivers/motor/m3508.hpp"
#include "drivers/protocol/can_comm.hpp"

// 选择要使用的 CAN 口: 可改成 &hcan2
static CanBus g_can(&hcan1);

// 创建4个M3508电机对象实例（用于处理反馈数据）
static M3508 motor1(&g_can, 1);
static M3508 motor2(&g_can, 2);
static M3508 motor3(&g_can, 3);
static M3508 motor4(&g_can, 4);

// 测试标志
static bool motors_initialized = false;

/**
 * @brief 初始化电机（配置 PID 参数）
 */
void init_motors() {
    if (motors_initialized) return;
    
    // 注册 CAN 回调
    motor1.init();
    motor2.init();
    motor3.init();
    motor4.init();
    
    // 配置速度环 PID 参数（M3508推荐：Kp=5~10, Ki=0.1~0.5）
    // 注意：Kp太小会导致启动力矩不足，电机不转
    motor1.setSpeedPID(8.0f, 0.3f, 0.0f);
    motor2.setSpeedPID(8.0f, 0.3f, 0.0f);
    motor3.setSpeedPID(8.0f, 0.3f, 0.0f);
    motor4.setSpeedPID(8.0f, 0.3f, 0.0f);
    
    // 配置位置环 PID 参数（位置环输出为速度，Kp=0.3~1.0）
    motor1.setPositionPID(0.5f, 0.0f, 0.1f);
    motor2.setPositionPID(0.5f, 0.0f, 0.1f);
    motor3.setPositionPID(0.5f, 0.0f, 0.1f);
    motor4.setPositionPID(0.5f, 0.0f, 0.1f);
    
    motors_initialized = true;
    
    // LED 指示初始化完成
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_11, GPIO_PIN_SET);
}

/**
 * @brief 速度环测试 - 单电机
 * @param target_speed 目标速度 (rpm)
 */
void test_speed_single(float target_speed) {
    // 设置电机 1 目标速度
    motor1.setTargetSpeed(target_speed);
    
    // 更新控制器
    motor1.update();
    
    // 打印调试信息（可选）
    // printf("Speed: %d rpm, Target: %.0f rpm\n", motor1.measure().speed_rpm, target_speed);
}

/**
 * @brief 速度环测试 - 四电机批量控制
 * @param s1-s4 各电机目标速度 (rpm)
 */
void test_speed_group(float s1, float s2, float s3, float s4) {
    // 批量设置目标速度
    M3508::setSpeedGroup(&motor1, &motor2, &motor3, &motor4, s1, s2, s3, s4);
    
    // 批量更新速度环
    M3508::updateSpeedGroup(&motor1, &motor2, &motor3, &motor4);
}

/**
 * @brief 位置环测试 - 单电机
 * @param target_position 目标位置（度）
 */
void test_position_single(float target_position) {
    // 设置电机 1 目标位置
    motor1.setTargetPosition(target_position);
    
    // 更新控制器
    motor1.update();
    
    // 打印调试信息（可选）
    // printf("Pos: %ld deg, Target: %.0f deg\n", motor1.measure().total_angle, target_position);
}

/**
 * @brief 位置环测试 - 四电机批量控制
 * @param p1-p4 各电机目标位置（度）
 */
void test_position_group(float p1, float p2, float p3, float p4) {
    // 批量设置目标位置
    M3508::setPositionGroup(&motor1, &motor2, &motor3, &motor4, p1, p2, p3, p4);
    
    // 批量更新位置环
    M3508::updatePositionGroup(&motor1, &motor2, &motor3, &motor4);
}

/**
 * @brief 停止所有电机
 */
void stop_all_motors() {
    M3508::sendCurrentGroup(&g_can, 0, 0, 0, 0);
}

/**
 * @brief 直接速度环测试（无需创建实例，直接传值）
 * @param target1-4 目标速度 (rpm)
 * @param kp PID 参数
 */
void test_speed_direct(float target1, float target2, float target3, float target4, float kp = 15.0f) {
    // 获取当前速度
    float current1 = static_cast<float>(motor1.measure().speed_rpm);
    float current2 = static_cast<float>(motor2.measure().speed_rpm);
    float current3 = static_cast<float>(motor3.measure().speed_rpm);
    float current4 = static_cast<float>(motor4.measure().speed_rpm);
    
    // 直接控制（简化 P 控制）
    M3508::controlSpeedDirect(&g_can, 
                             target1, target2, target3, target4,
                             current1, current2, current3, current4,
                             kp, 0.0f, 0.0f, 1);
}

/**
 * @brief 直接位置环测试（无需创建实例，直接传值）
 * @param target1-4 目标位置（度）
 * @param pos_kp, spd_kp PID 参数
 */
void test_position_direct(float target1, float target2, float target3, float target4, 
                         float pos_kp = 0.8f, float spd_kp = 15.0f) {
    // 获取当前位置和速度
    float pos1 = static_cast<float>(motor1.measure().total_angle);
    float pos2 = static_cast<float>(motor2.measure().total_angle);
    float pos3 = static_cast<float>(motor3.measure().total_angle);
    float pos4 = static_cast<float>(motor4.measure().total_angle);
    
    float spd1 = static_cast<float>(motor1.measure().speed_rpm);
    float spd2 = static_cast<float>(motor2.measure().speed_rpm);
    float spd3 = static_cast<float>(motor3.measure().speed_rpm);
    float spd4 = static_cast<float>(motor4.measure().speed_rpm);
    
    // 直接控制（简化级联 P 控制）
    M3508::controlPositionDirect(&g_can,
                                target1, target2, target3, target4,
                                pos1, pos2, pos3, pos4,
                                spd1, spd2, spd3, spd4,
                                pos_kp, 0.0f, 0.0f,
                                spd_kp, 0.0f, 0.0f,
                                1);
}

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void test_speed_manual_kp(float target_speed, float kp) {
    while (g_can.pollOnce()) {}
    float current_speed = static_cast<float>(motor1.measure().speed_rpm);
    float out = (target_speed - current_speed) * kp;
    out = clampf(out, -16384.0f, 16384.0f);
    motor1.setCurrent(static_cast<int16_t>(out));
}

static inline void put16_be(uint8_t* p, uint8_t off, int16_t v) {
    p[off] = (uint8_t)(v >> 8);
    p[off + 1] = (uint8_t)(v);
}

void test_speed_kp_raw(float target_speed, float kp, uint8_t motor_id = 1) {
    while (g_can.pollOnce()) {}
    int16_t spd = 0;
    switch (motor_id) {
        case 1: spd = motor1.measure().speed_rpm; break;
        case 2: spd = motor2.measure().speed_rpm; break;
        case 3: spd = motor3.measure().speed_rpm; break;
        case 4: spd = motor4.measure().speed_rpm; break;
        default: spd = motor1.measure().speed_rpm; break;
    }
    float outf = (target_speed - (float)spd) * kp;
    if (outf > 16384.0f) outf = 16384.0f; if (outf < -16384.0f) outf = -16384.0f;
    int16_t out = (int16_t)outf;
    uint8_t data[8] = {0};
    uint16_t can_id;
    uint8_t slot_off;
    if (motor_id >= 1 && motor_id <= 4) {
        can_id = M3508::kGroupCurrent;
        slot_off = (uint8_t)((motor_id - 1) * 2);
    } else {
        can_id = M3508::kGroupCurrent2;
        uint8_t idx = (uint8_t)((motor_id - 5) & 0x03);
        slot_off = (uint8_t)(idx * 2);
    }
    put16_be(data, slot_off, out);
    g_can.sendStd(can_id, data, 8);
}

/**
 * @brief HAL库 CAN接收FIFO0消息挂起回调（中断处理）
 */
extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    if (hcan == g_can.handle()) {
        // 轮询接收，触发已注册的回调
        g_can.pollOnce();
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_CAN1_Init();
    MX_CAN2_Init();
    can_filter_init();

    // 延迟等待电机上电稳定
    HAL_Delay(200);
    

    // 初始化电机
    init_motors();

    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    
    // LED 指示就绪
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_10, GPIO_PIN_SET);
    
    // ========== 测试选择 ==========
    // 取消注释你想测试的部分
    
    // 测试 1: 单电机速度环测试
    // while (1) {
    //     test_speed_manual_kp(500.0f, 8.0f);
    //     HAL_Delay(1);
    // }
    
    // 测试 2: 四电机速度环批量测试
    // while (1) {
    //     test_speed_group(1500.0f, -500.0f, 800.0f, -800.0f);
    //     HAL_Delay(1);
    // }
    
    // 测试 3: 单电机位置环测试（往复运动）
    // while (1) {
    //     test_position_single(360.0f);   // 转到 360 度
    //     HAL_Delay(2000);
    //     test_position_single(300.0f);   
    //     HAL_Delay(2000);
    //     test_position_single(240.0f);   
    //     HAL_Delay(2000);
    //     test_position_single(180.0f);   
    //     HAL_Delay(2000);
    //     test_position_single(120.0f);   
    //     HAL_Delay(2000);
    //     test_position_single(60.0f);   
    //     HAL_Delay(2000);
    //     test_position_single(0.0f);     // 转回 0 度
    //     HAL_Delay(2000);
    // }
    
    // 测试 4: 四电机位置环批量测试
    // while (1) {
    //     test_position_group(0.0f, 0.0f, 0.0f, 0.0f);
    //     HAL_Delay(2000);
    //     test_position_group(360.0f, 720.0f, 180.0f, 540.0f);
    //     HAL_Delay(2000);
    // }
    
    // 测试 5: 直接速度环测试（无需实例，直接传值）
    // 注意：Kp建议使用5.0-10.0，太小会导致电机不转
    while (1) {
        test_speed_direct(1000.0f, -100.0f, 200.0f, -200.0f, 8.0f);  // 最后一个参数是Kp
        HAL_Delay(1);
    }
    
    // 测试 6: 直接位置环测试（无需实例，直接传值）
    // 注意：这是简化的P控制，已添加死区判断（±5°）避免震荡
    // 推荐参数：pos_kp=0.5, spd_kp=8.0
    // while (1) {
    //     test_position_direct(0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 8.0f);
    //     HAL_Delay(3000);
    //     test_position_direct(360.0f, 720.0f, 180.0f, 540.0f, 0.5f, 8.0f);
    //     HAL_Delay(3000);
    // }
    
    // 测试 7: 速度环变速测试
    // while (1) {
    //     // 加速
    //     for (int i = 0; i <= 1000; i += 100) {
    //         test_speed_single((float)i);
    //         HAL_Delay(500);
    //     }
    //     // 减速
    //     for (int i = 1000; i >= 0; i -= 100) {
    //         test_speed_single((float)i);
    //         HAL_Delay(500);
    //     }
    //     // 反向
    //     for (int i = 0; i >= -1000; i -= 100) {
    //         test_speed_single((float)i);
    //         HAL_Delay(500);
    //     }
    //     // 回零
    //     for (int i = -1000; i <= 0; i += 100) {
    //         test_speed_single((float)i);
    //         HAL_Delay(500);
    //     }
    // }
}

extern "C" void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 6;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
            RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

extern "C" void Error_Handler(void) {
    // 可在此处添加调试行为（如点灯）
    while (1) {
    }
}
