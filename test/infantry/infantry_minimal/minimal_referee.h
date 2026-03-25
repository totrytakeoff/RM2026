/**
 * @file minimal_referee.h
 * @brief 最小裁判系统只读封装(联锁用)
 */

#ifndef MINIMAL_REFEREE_H
#define MINIMAL_REFEREE_H

#include "minimal_referee_types.h"

void MinimalReferee_Init(void);
void MinimalReferee_Update(void);
const MinimalRefereeData_t *MinimalReferee_GetData(void);

uint8_t MinimalReferee_AllowChassis(void);
uint8_t MinimalReferee_AllowShoot(void);
uint8_t MinimalReferee_AllowLoader(void);
float MinimalReferee_ChassisScale(void);
float MinimalReferee_FrictionSpeedScale(void);

#endif /* MINIMAL_REFEREE_H */
