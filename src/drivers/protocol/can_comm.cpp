/**
 * @file can_comm.cpp
 * @brief CanBus 实现（与 can_comm.hpp 配对）
 */

extern "C" {
#include "stm32f4xx_hal.h"
}

#include "can_comm.hpp"

CanBus::CanBus(CAN_HandleTypeDef* handle)
    : h_(handle), rx_cb_(0), rx_user_(0) {}

void CanBus::attachRxCallback(RxCallback cb, void* user) {
    rx_cb_ = cb;
    rx_user_ = user;
}

HAL_StatusTypeDef CanBus::sendStd(uint16_t std_id, const uint8_t* data, uint8_t len) {
    if (!h_) return HAL_ERROR;
    CAN_TxHeaderTypeDef tx = {0};
    uint32_t mailbox = 0;
    tx.StdId = std_id;
    tx.ExtId = 0;
    tx.IDE = CAN_ID_STD;
    tx.RTR = CAN_RTR_DATA;
    tx.DLC = len & 0x0F;
    return HAL_CAN_AddTxMessage(h_, &tx, const_cast<uint8_t*>(data), &mailbox);
}

HAL_StatusTypeDef CanBus::sendExt(uint32_t ext_id, const uint8_t* data, uint8_t len) {
    if (!h_) return HAL_ERROR;
    CAN_TxHeaderTypeDef tx = {0};
    uint32_t mailbox = 0;
    tx.StdId = 0;
    tx.ExtId = ext_id;
    tx.IDE = CAN_ID_EXT;
    tx.RTR = CAN_RTR_DATA;
    tx.DLC = len & 0x0F;
    return HAL_CAN_AddTxMessage(h_, &tx, const_cast<uint8_t*>(data), &mailbox);
}

bool CanBus::pollOnce(void) {
    if (!h_) return false;
    if (HAL_CAN_GetRxFifoFillLevel(h_, CAN_RX_FIFO0) == 0) return false;
    CAN_RxHeaderTypeDef rxh;
    uint8_t buf[8];
    if (HAL_CAN_GetRxMessage(h_, CAN_RX_FIFO0, &rxh, buf) != HAL_OK) return false;
    if (rx_cb_) rx_cb_(&rxh, buf, rx_user_);
    return true;
}

CAN_HandleTypeDef* CanBus::handle() { return h_; }
