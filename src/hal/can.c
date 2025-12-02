#include "can.h"
#include "main.h"


CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

void MX_CAN1_Init(void)
{
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 3;                 // 1Mbps @ APB1 42MHz, tq = (Prescaler)/42MHz
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_10TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_3TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  // 关键修复：启用自动错误恢复与重发
  // 场景说明：若MCU先启动而电机电调未上电，CAN发送无ACK会导致错误计数攀升并进入Bus-Off。
  // 若未启用自动恢复/唤醒/重发，之后即便电机再上电，CAN控制器也不会自动恢复，
  // 需要“重启开发板”或手动Stop/Start才能恢复。
  // 解决方案：打开以下选项以实现自动恢复与可靠收发。
  hcan1.Init.AutoBusOff = ENABLE;       // 允许进入Bus-Off后自动恢复
  hcan1.Init.AutoWakeUp = ENABLE;       // 总线活动恢复后自动唤醒控制器
  hcan1.Init.AutoRetransmission = ENABLE; // 无ACK等情况自动重发，提高可靠性
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
}

void MX_CAN2_Init(void)
{
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 3;                 // 1Mbps
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_10TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_3TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  // 同CAN1：启用自动恢复/唤醒/重发，避免“电机断电-后上电”导致的不可用状态
  hcan2.Init.AutoBusOff = ENABLE;
  hcan2.Init.AutoWakeUp = ENABLE;
  hcan2.Init.AutoRetransmission = ENABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
}

static uint32_t HAL_RCC_CAN1_CLK_ENABLED = 0;

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (canHandle->Instance == CAN1)
  {
    HAL_RCC_CAN1_CLK_ENABLED++;
    if (HAL_RCC_CAN1_CLK_ENABLED == 1) {
      __HAL_RCC_CAN1_CLK_ENABLE();
    }

    __HAL_RCC_GPIOD_CLK_ENABLE();
    // PD0 -> CAN1_RX, PD1 -> CAN1_TX
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  }
  else if (canHandle->Instance == CAN2)
  {
    __HAL_RCC_CAN2_CLK_ENABLE();
    HAL_RCC_CAN1_CLK_ENABLED++;
    if (HAL_RCC_CAN1_CLK_ENABLED == 1) {
      __HAL_RCC_CAN1_CLK_ENABLE();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    // PB5 -> CAN2_RX, PB6 -> CAN2_TX
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{
  if (canHandle->Instance == CAN1)
  {
    HAL_RCC_CAN1_CLK_ENABLED--;
    if (HAL_RCC_CAN1_CLK_ENABLED == 0) {
      __HAL_RCC_CAN1_CLK_DISABLE();
    }
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_0 | GPIO_PIN_1);
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
  }
  else if (canHandle->Instance == CAN2)
  {
    __HAL_RCC_CAN2_CLK_DISABLE();
    HAL_RCC_CAN1_CLK_ENABLED--;
    if (HAL_RCC_CAN1_CLK_ENABLED == 0) {
      __HAL_RCC_CAN1_CLK_DISABLE();
    }
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5 | GPIO_PIN_6);
    HAL_NVIC_DisableIRQ(CAN2_RX0_IRQn);
  }
}

void can_filter_init(void)
{
  // Configure CAN1 filter: accept all, FIFO0, bank 0
  CAN_FilterTypeDef filter = {0};
  filter.FilterActivation = ENABLE;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;
  filter.FilterIdHigh = 0x0000;
  filter.FilterIdLow = 0x0000;
  filter.FilterMaskIdHigh = 0x0000;
  filter.FilterMaskIdLow = 0x0000;
  filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  filter.FilterBank = 0;                 // Banks 0-13 for CAN1
  filter.SlaveStartFilterBank = 14;      // Banks 14-27 for CAN2
  HAL_CAN_ConfigFilter(&hcan1, &filter);

  // Configure CAN2 filter: accept all, FIFO0, bank 14
  filter.FilterBank = 14;
  HAL_CAN_ConfigFilter(&hcan2, &filter);

  // 启动 CAN 并启用常用中断：
  // 1) RX FIFO0 新报文挂起（触发应用层轮询/回调，保证反馈及时更新）
  // 2) 错误、中断、Bus-Off（便于应用层必要时感知异常并记录日志）
  HAL_CAN_Start(&hcan1);
  HAL_CAN_ActivateNotification(&hcan1, 
      CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_BUSOFF | CAN_IT_ERROR);

  HAL_CAN_Start(&hcan2);
  HAL_CAN_ActivateNotification(&hcan2, 
      CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_BUSOFF | CAN_IT_ERROR);
}