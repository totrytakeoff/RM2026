/**
 * @file infantry_input.h
 * @brief 统一输入模块 - 支持ET08遥控和VT图传键鼠
 */

#ifndef INFANTRY_INPUT_H
#define INFANTRY_INPUT_H

#include <stdbool.h>

#include "infantry_types.h"

/**
 * @brief 输入模块初始化
 */
bool Input_Init(void);
void Input_UpdateET08(Input_Data_t *et08_data);
void Input_UpdateVT(Input_Data_t *vt_data);
void Input_Arbitrate(const Input_Data_t *vt_data, const Input_Data_t *et08_data, Input_Data_t *out);

/**
 * @brief 获取统一输入数据
 * @param data 输出数据结构指针
 */
void Input_GetData(Input_Data_t *data);

/**
 * @brief 检查输入源是否在线
 * @return 1在线, 0离线
 */
uint8_t Input_IsOnline(void);
uint8_t Input_IsVTAllowed(void);

#endif /* INFANTRY_INPUT_H */
