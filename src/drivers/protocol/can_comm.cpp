/**
 * @file can_comm.cpp
 * @brief CanBus 实现（与 can_comm.hpp 配对）
 */

extern "C" {
#include "stm32f4xx_hal.h"
}

#include "can_comm.hpp"

CanBus::CanBus(CAN_HandleTypeDef* handle)
    : h_(handle), callback_list_(nullptr), callback_count_(0) {
    // 初始化回调节点池
    for (uint8_t i = 0; i < MAX_CALLBACKS; ++i) {
        callback_pool_[i].callback = nullptr;
        callback_pool_[i].user_data = nullptr;
        callback_pool_[i].filter_id = 0;
        callback_pool_[i].use_filter = false;
        callback_pool_[i].is_ext_id = false;
        callback_pool_[i].next = nullptr;
    }
}

bool CanBus::registerRxCallback(RxCallback cb, void* user) {
    if (!cb || callback_count_ >= MAX_CALLBACKS) {
        return false;
    }
    
    // 从池中获取一个空闲节点
    CallbackNode* node = nullptr;
    for (uint8_t i = 0; i < MAX_CALLBACKS; ++i) {
        if (callback_pool_[i].callback == nullptr) {
            node = &callback_pool_[i];
            break;
        }
    }
    
    if (!node) return false;
    
    // 配置节点
    node->callback = cb;
    node->user_data = user;
    node->use_filter = false;
    node->filter_id = 0;
    node->is_ext_id = false;
    
    // 插入链表头部
    node->next = callback_list_;
    callback_list_ = node;
    callback_count_++;
    
    return true;
}

bool CanBus::registerRxCallback(RxCallback cb, uint32_t filter_id, bool is_ext_id, void* user) {
    if (!cb || callback_count_ >= MAX_CALLBACKS) {
        return false;
    }
    
    // 从池中获取一个空闲节点
    CallbackNode* node = nullptr;
    for (uint8_t i = 0; i < MAX_CALLBACKS; ++i) {
        if (callback_pool_[i].callback == nullptr) {
            node = &callback_pool_[i];
            break;
        }
    }
    
    if (!node) return false;
    
    // 配置节点（带过滤）
    node->callback = cb;
    node->user_data = user;
    node->use_filter = true;
    node->filter_id = filter_id;
    node->is_ext_id = is_ext_id;
    
    // 插入链表头部
    node->next = callback_list_;
    callback_list_ = node;
    callback_count_++;
    
    return true;
}

bool CanBus::unregisterRxCallback(RxCallback cb) {
    if (!cb || !callback_list_) {
        return false;
    }
    
    CallbackNode* prev = nullptr;
    CallbackNode* curr = callback_list_;
    
    // 遍历链表查找匹配的回调
    while (curr) {
        if (curr->callback == cb) {
            // 从链表中移除
            if (prev) {
                prev->next = curr->next;
            } else {
                callback_list_ = curr->next;
            }
            
            // 清空节点（返回池中）
            curr->callback = nullptr;
            curr->user_data = nullptr;
            curr->filter_id = 0;
            curr->use_filter = false;
            curr->is_ext_id = false;
            curr->next = nullptr;
            
            callback_count_--;
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    
    return false;
}

void CanBus::clearAllCallbacks() {
    CallbackNode* curr = callback_list_;
    while (curr) {
        CallbackNode* next = curr->next;
        
        // 清空节点
        curr->callback = nullptr;
        curr->user_data = nullptr;
        curr->filter_id = 0;
        curr->use_filter = false;
        curr->is_ext_id = false;
        curr->next = nullptr;
        
        curr = next;
    }
    
    callback_list_ = nullptr;
    callback_count_ = 0;
}

uint8_t CanBus::getCallbackCount() const {
    return callback_count_;
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
    
    // 触发所有匹配的回调
    invokeCallbacks(&rxh, buf);
    
    return true;
}

void CanBus::invokeCallbacks(const CAN_RxHeaderTypeDef* header, const uint8_t* data) {
    if (!header || !data) return;
    
    CallbackNode* curr = callback_list_;
    while (curr) {
        bool should_invoke = false;
        
        if (!curr->use_filter) {
            // 无过滤，接收所有帧
            should_invoke = true;
        } else {
            // 检查 ID 是否匹配
            if (curr->is_ext_id && header->IDE == CAN_ID_EXT) {
                should_invoke = (header->ExtId == curr->filter_id);
            } else if (!curr->is_ext_id && header->IDE == CAN_ID_STD) {
                should_invoke = (header->StdId == curr->filter_id);
            }
        }
        
        // 调用回调
        if (should_invoke && curr->callback) {
            curr->callback(header, data, curr->user_data);
        }
        
        curr = curr->next;
    }
}

CAN_HandleTypeDef* CanBus::handle() { 
    return h_; 
}
