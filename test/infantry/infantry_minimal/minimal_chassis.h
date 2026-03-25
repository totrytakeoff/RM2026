/**
 * @file minimal_chassis.h
 * @brief 底盘控制模块
 */

#ifndef MINIMAL_CHASSIS_H
#define MINIMAL_CHASSIS_H

#include "minimal_types.h"

/**
 * @brief 底盘初始化
 */
void Chassis_Init(void);

/**
 * @brief 底盘更新
 * @param input 输入数据
 */
void Chassis_Update(Input_Data_t *input);

/**
 * @brief 底盘停止
 */
void Chassis_Stop(void);

/**
 * @brief 获取底盘旋转角速度(供云台使用)
 * @return 旋转角速度 rad/s
 */
float Chassis_GetWz(void);
float Chassis_GetFRSpeedRef(void);
float Chassis_GetFRSpeedFdb(void);
float Chassis_GetPowerScale(void);

#endif /* MINIMAL_CHASSIS_H */
