#ifndef __CAN_H__
#define __CAN_H__

#include "stm32f4xx_hal.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

void MX_CAN1_Init(void);
void MX_CAN2_Init(void);
void can_filter_init(void);

/*

can_filter_init()
 的作用
配置过滤器
使用 32 位掩码模式，Id/Mask 全零，等价于“接受所有 ID”（标准/扩展都会放行）
过滤器分配：
CAN1 使用 bank 0
CAN2 使用 bank 14
SlaveStartFilterBank = 14 表示 013 归 CAN1，1427 归 CAN2（F4 的 CAN1/2 共享滤波器资源，需手动切分）
FilterFIFOAssignment = CAN_FILTER_FIFO0：放进 FIFO0
启动外设与接收中断
HAL_CAN_Start(&hcanX)：启动 CAN 外设
注意：未 Start 时，既不能收也不能发（TX 也需要 START）
HAL_CAN_ActivateNotification(&hcanX, CAN_IT_RX_FIFO0_MSG_PENDING)：打开 FIFO0 收到消息的中断
这样 
HAL_CAN_RxFifo0MsgPendingCallback()
 才会被调用，
CAN_receive.c
 才能解析电机反馈
为什么必要
如果只是“发送”，也需要 HAL_CAN_Start()；不 Start 会导致 HAL_CAN_AddTxMessage 不工作
如果需要“接收反馈”，必须配置过滤器+启用通知，否则不会进回调

*/

#endif
