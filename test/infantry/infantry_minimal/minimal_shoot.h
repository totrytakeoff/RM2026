/**
 * @file minimal_shoot.h
 * @brief 发射控制模块
 */

#ifndef MINIMAL_SHOOT_H
#define MINIMAL_SHOOT_H

#include "minimal_types.h"

/*============================================================================
 * 发射状态枚举
 *============================================================================*/
typedef enum {
    SHOOT_OFF = 0,          // 关闭
    SHOOT_FRICTION_ON,      // 仅摩擦轮
    SHOOT_SINGLE,           // 单发
    SHOOT_CONTINUOUS,       // 连发
} ShootState_e;

/**
 * @brief 发射初始化
 */
void Shoot_Init(void);

/**
 * @brief 发射更新
 * @param input 输入数据
 */
void Shoot_Update(Input_Data_t *input);

/**
 * @brief 发射停止
 */
void Shoot_Stop(void);
ShootState_e Shoot_GetState(void);
float Shoot_GetLoaderRef(void);
float Shoot_GetLoaderFeedback(void);
uint8_t Shoot_IsSingleActive(void);
uint8_t Shoot_GetPendingShots(void);

#endif /* MINIMAL_SHOOT_H */
