/**
 * @file M3508_advanced_demo.cpp
 * @brief M3508 电机高级控制测试程序（速度环+位置环）
 * @author RM2026 Team
 * @date 2025-12-02
 * 
 * 功能：
 * - 测试 CAN 回调列表管理
 * - 测试 M3508 速度环控制
 * - 测试 M3508 位置环控制
 * - 演示多电机协同控制
 */

extern "C" {
#include "stm32f4xx_hal.h"
#include "can.h"
#include "bsp/bsp_board.h"
#include "bsp/bsp_led.h"
}

#include "drivers/protocol/can_comm.hpp"
#include "drivers/motor/m3508.hpp"

// 全局 CAN 总线对象
static CanBus g_can1(&hcan1);

// 4 个 M3508 电机对象
static M3508 motor1(&g_can1, 1);
static M3508 motor2(&g_can1, 2);
static M3508 motor3(&g_can1, 3);
static M3508 motor4(&g_can1, 4);

// 测试模式
enum TestMode {
    TEST_OPEN_LOOP = 0,     // 开环测试
    TEST_SPEED_LOOP = 1,    // 速度环测试
    TEST_POSITION_LOOP = 2, // 位置环测试
    TEST_MULTI_MOTOR = 3    // 多电机协同测试
};

static TestMode current_mode = TEST_OPEN_LOOP;
static uint32_t mode_start_time = 0;
static const uint32_t MODE_DURATION = 10000; // 每个模式持续 10 秒

/**
 * @brief 初始化 CAN 和电机
 */
void init_motors() {
    // 初始化电机（注册 CAN 回调）
    if (!motor1.init()) {
        BSP_LED_SetPresetColor(BSP_LED_COLOR_RED);
        while(1); // 初始化失败
    }
    motor2.init();
    motor3.init();
    motor4.init();
    
    // 配置速度环 PID 参数
    motor1.setSpeedPID(15.0f, 0.8f, 0.0f);
    motor2.setSpeedPID(15.0f, 0.8f, 0.0f);
    motor3.setSpeedPID(15.0f, 0.8f, 0.0f);
    motor4.setSpeedPID(15.0f, 0.8f, 0.0f);
    
    // 配置位置环 PID 参数
    motor1.setPositionPID(0.8f, 0.0f, 0.2f);
    motor2.setPositionPID(0.8f, 0.0f, 0.2f);
    motor3.setPositionPID(0.8f, 0.0f, 0.2f);
    motor4.setPositionPID(0.8f, 0.0f, 0.2f);
    
    BSP_LED_SetPresetColor(BSP_LED_COLOR_GREEN);
}

/**
 * @brief 开环控制测试
 */
void test_open_loop() {
    static uint32_t last_update = 0;
    uint32_t now = HAL_GetTick();
    
    if (now - last_update >= 1000) {
        last_update = now;
        
        // 电机 1: 正转
        motor1.setControlMode(M3508::ControlMode::OPEN_LOOP);
        motor1.setCurrent(5000);
        
        // 电机 2: 反转
        motor2.setControlMode(M3508::ControlMode::OPEN_LOOP);
        motor2.setCurrent(-5000);
        
        // 电机 3 和 4: 停止
        motor3.setControlMode(M3508::ControlMode::OPEN_LOOP);
        motor3.setCurrent(0);
        motor4.setControlMode(M3508::ControlMode::OPEN_LOOP);
        motor4.setCurrent(0);
        
        // LED 指示：蓝色
        BSP_LED_SetPresetColor(BSP_LED_COLOR_BLUE);
    }
}

/**
 * @brief 速度环控制测试
 */
void test_speed_loop() {
    static uint32_t last_update = 0;
    static bool direction = true;
    uint32_t now = HAL_GetTick();
    
    // 每 2 秒切换一次方向
    if (now - last_update >= 2000) {
        last_update = now;
        direction = !direction;
        
        float target_speed = direction ? 1000.0f : -1000.0f;
        
        // 设置所有电机目标速度
        motor1.setTargetSpeed(target_speed);
        motor2.setTargetSpeed(target_speed);
        motor3.setTargetSpeed(target_speed * 0.5f);
        motor4.setTargetSpeed(target_speed * 0.5f);
        
        // LED 指示：黄色
        BSP_LED_SetRGB(255, 255, 0);
    }
    
    // 更新控制器（1ms 周期）
    motor1.update();
    motor2.update();
    motor3.update();
    motor4.update();
}

/**
 * @brief 位置环控制测试
 */
void test_position_loop() {
    static uint32_t last_update = 0;
    static uint8_t step = 0;
    uint32_t now = HAL_GetTick();
    
    // 每 3 秒切换一次目标位置
    if (now - last_update >= 3000) {
        last_update = now;
        step = (step + 1) % 4;
        
        float target_positions[4] = {0.0f, 360.0f, 720.0f, 1080.0f};
        float target = target_positions[step];
        
        // 设置所有电机目标位置
        motor1.setTargetPosition(target);
        motor2.setTargetPosition(target);
        motor3.setTargetPosition(target * 0.5f);
        motor4.setTargetPosition(target * 0.5f);
        
        // LED 指示：紫色
        BSP_LED_SetRGB(255, 0, 255);
    }
    
    // 更新控制器（1ms 周期）
    motor1.update();
    motor2.update();
    motor3.update();
    motor4.update();
}

/**
 * @brief 多电机协同测试
 */
void test_multi_motor() {
    static uint32_t last_update = 0;
    uint32_t now = HAL_GetTick();
    
    // 电机 1 和 2: 速度环控制
    motor1.setControlMode(M3508::ControlMode::SPEED_LOOP);
    motor2.setControlMode(M3508::ControlMode::SPEED_LOOP);
    
    // 电机 3 和 4: 位置环控制
    motor3.setControlMode(M3508::ControlMode::POSITION_LOOP);
    motor4.setControlMode(M3508::ControlMode::POSITION_LOOP);
    
    if (now - last_update >= 100) {
        last_update = now;
        
        // 电机 1 和 2: 正弦波速度
        float time_sec = now / 1000.0f;
        float speed1 = 500.0f * sinf(time_sec);
        float speed2 = 500.0f * cosf(time_sec);
        
        motor1.setTargetSpeed(speed1);
        motor2.setTargetSpeed(speed2);
        
        // 电机 3 和 4: 往复运动
        float pos = 360.0f * sinf(time_sec * 0.5f);
        motor3.setTargetPosition(pos);
        motor4.setTargetPosition(-pos);
        
        // LED 指示：青色
        BSP_LED_SetRGB(0, 255, 255);
    }
    
    // 更新所有控制器
    motor1.update();
    motor2.update();
    motor3.update();
    motor4.update();
}

/**
 * @brief 主循环
 */
void run_test_loop() {
    uint32_t now = HAL_GetTick();
    
    // 轮询 CAN 接收（处理所有回调）
    while (g_can1.pollOnce()) {
        // 持续轮询直到 FIFO 为空
    }
    
    // 检查是否需要切换测试模式
    if (now - mode_start_time >= MODE_DURATION) {
        mode_start_time = now;
        current_mode = static_cast<TestMode>((current_mode + 1) % 4);
        
        // 切换模式时重置所有电机
        motor1.reset();
        motor2.reset();
        motor3.reset();
        motor4.reset();
        
        // LED 闪烁指示模式切换
        BSP_LED_SetPresetColor(BSP_LED_COLOR_WHITE);
        BSP_Delay(200);
    }
    
    // 根据当前模式执行测试
    switch (current_mode) {
        case TEST_OPEN_LOOP:
            test_open_loop();
            break;
            
        case TEST_SPEED_LOOP:
            test_speed_loop();
            break;
            
        case TEST_POSITION_LOOP:
            test_position_loop();
            break;
            
        case TEST_MULTI_MOTOR:
            test_multi_motor();
            break;
    }
    
    // 控制循环频率（1ms）
    HAL_Delay(1);
}

/**
 * @brief 主函数
 */
int main(void) {
    // BSP 初始化
    BSP_InitTypeDef config = BSP_INIT_DEFAULT;
    if (BSP_Init(&config) != BSP_OK) {
        while(1);
    }
    
    BSP_LED_Init();
    BSP_LED_SetPresetColor(BSP_LED_COLOR_YELLOW);
    BSP_Delay(500);
    
    // 初始化电机
    init_motors();
    
    mode_start_time = HAL_GetTick();
    
    // 主循环
    while (1) {
        run_test_loop();
    }
    
    return 0;
}
