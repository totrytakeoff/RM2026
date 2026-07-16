/**
 * @file infantry_chassis.h
 * @brief 底盘控制模块
 */

#ifndef INFANTRY_CHASSIS_H
#define INFANTRY_CHASSIS_H

#include <stdbool.h>

#include "infantry_types.h"

/**
 * @brief 底盘初始化
 */
bool Chassis_Init(void);

/**
 * @brief 底盘更新
 * @param input 输入数据
 */
void Chassis_Update(Input_Data_t *input);

/**
 * @brief 底盘停止
 */
void Chassis_Stop(void);

/** True when all required chassis motor feedback is inside its deadline. */
bool Chassis_IsHealthy(void);

/**
 * @brief 获取底盘旋转角速度(供云台使用)
 * @return 旋转角速度 rad/s
 */
float Chassis_GetWz(void);
float Chassis_GetFRSpeedRef(void);
float Chassis_GetFRSpeedFdb(void);
float Chassis_GetPowerScale(void);

#endif /* INFANTRY_CHASSIS_H */
