/**
 * @file infantry_shoot.h
 * @brief 发射控制模块
 */

#ifndef INFANTRY_SHOOT_H
#define INFANTRY_SHOOT_H

#include <stdbool.h>

#include "infantry_types.h"

#ifdef __cplusplus
extern "C" {
#endif

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
bool Shoot_Init(void);

/**
 * @brief 发射更新
 * @param input 输入数据
 */
void Shoot_Update(Input_Data_t *input);

/**
 * @brief 发射停止
 */
void Shoot_Stop(void);
bool Shoot_IsHealthy(void);
ShootState_e Shoot_GetState(void);
float Shoot_GetLoaderRef(void);
float Shoot_GetLoaderFeedback(void);
uint8_t Shoot_IsSingleActive(void);
uint8_t Shoot_GetPendingShots(void);

#ifdef __cplusplus
}
#endif

#endif /* INFANTRY_SHOOT_H */
