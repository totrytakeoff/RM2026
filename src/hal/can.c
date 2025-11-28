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
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
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
  hcan2.Init.AutoBusOff = DISABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = DISABLE;
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

  // Start CAN but暂时禁用接收中断，避免GM6020上电时的中断风暴
  HAL_CAN_Start(&hcan1);
  // HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING); // 暂时注释掉

  HAL_CAN_Start(&hcan2);
  // HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING); // 暂时注释掉
}
