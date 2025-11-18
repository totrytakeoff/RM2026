#ifndef __CAN_H__
#define __CAN_H__

#include "stm32f4xx_hal.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

void MX_CAN1_Init(void);
void MX_CAN2_Init(void);
void can_filter_init(void);

#endif
