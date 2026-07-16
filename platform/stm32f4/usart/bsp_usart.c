#include "bsp_usart.h"

#include <stddef.h>
#include <string.h>

#include "bsp_log.h"

static USARTInstance usart_instances[USART_MAX_INSTANCES];
static size_t usart_instance_count;

uint8_t USARTServiceInit(USARTInstance *instance)
{
    HAL_StatusTypeDef status;

    if ((instance == NULL) || (instance->usart_handle == NULL) ||
        (instance->usart_handle->hdmarx == NULL) ||
        (instance->recv_buff_size == 0U)) {
        return 0U;
    }

    status = HAL_UARTEx_ReceiveToIdle_DMA(instance->usart_handle,
                                         instance->recv_buff,
                                         instance->recv_buff_size);
    if (status == HAL_BUSY) {
        if (HAL_UART_AbortReceive(instance->usart_handle) != HAL_OK) {
            return 0U;
        }
        status = HAL_UARTEx_ReceiveToIdle_DMA(instance->usart_handle,
                                             instance->recv_buff,
                                             instance->recv_buff_size);
    }
    if (status != HAL_OK) {
        return 0U;
    }

    __HAL_DMA_DISABLE_IT(instance->usart_handle->hdmarx, DMA_IT_HT);
    return 1U;
}

USARTInstance *USARTRegister(const USART_Init_Config_s *config)
{
    USARTInstance *instance;

    if ((config == NULL) || (config->usart_handle == NULL) ||
        (config->recv_buff_size == 0U) ||
        (config->recv_buff_size > USART_RX_BUFFER_CAPACITY)) {
        return NULL;
    }
    if (usart_instance_count >= USART_MAX_INSTANCES) {
        LOGERROR("[bsp_usart] USART instance capacity exhausted");
        return NULL;
    }

    for (size_t i = 0U; i < usart_instance_count; ++i) {
        if (usart_instances[i].usart_handle == config->usart_handle) {
            LOGERROR("[bsp_usart] UART already registered");
            return NULL;
        }
    }

    instance = &usart_instances[usart_instance_count];
    memset(instance, 0, sizeof(*instance));
    instance->usart_handle = config->usart_handle;
    instance->recv_buff_size = config->recv_buff_size;
    instance->module_callback = config->module_callback;
    usart_instance_count++;

    if (!USARTServiceInit(instance)) {
        usart_instance_count--;
        memset(instance, 0, sizeof(*instance));
        LOGERROR("[bsp_usart] failed to start receive DMA");
        return NULL;
    }

    return instance;
}

uint8_t USARTSend(USARTInstance *instance,
                  uint8_t *send_buf,
                  uint16_t send_size,
                  USART_TRANSFER_MODE mode)
{
    HAL_StatusTypeDef status;

    if ((instance == NULL) || (instance->usart_handle == NULL) ||
        (send_buf == NULL) || (send_size == 0U)) {
        return 0U;
    }

    switch (mode) {
    case USART_TRANSFER_BLOCKING:
        status = HAL_UART_Transmit(instance->usart_handle,
                                   send_buf,
                                   send_size,
                                   USART_BLOCKING_TX_TIMEOUT_MS);
        break;
    case USART_TRANSFER_IT:
        status = HAL_UART_Transmit_IT(instance->usart_handle,
                                     send_buf,
                                     send_size);
        break;
    case USART_TRANSFER_DMA:
        status = HAL_UART_Transmit_DMA(instance->usart_handle,
                                      send_buf,
                                      send_size);
        break;
    case USART_TRANSFER_NONE:
    default:
        return 0U;
    }

    return (status == HAL_OK) ? 1U : 0U;
}

uint8_t USARTIsReady(const USARTInstance *instance)
{
    if ((instance == NULL) || (instance->usart_handle == NULL)) {
        return 0U;
    }

    return (instance->usart_handle->gState == HAL_UART_STATE_READY) ? 1U : 0U;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *handle, uint16_t size)
{
    for (size_t i = 0U; i < usart_instance_count; ++i) {
        USARTInstance *instance = &usart_instances[i];
        uint16_t received_size;

        if (instance->usart_handle != handle) {
            continue;
        }

        received_size = (size <= instance->recv_buff_size)
                            ? size
                            : instance->recv_buff_size;
        instance->recv_len = received_size;
        instance->rx_event_count++;
        if (instance->module_callback != NULL) {
            instance->module_callback();
        }
        memset(instance->recv_buff, 0, received_size);
        if (!USARTServiceInit(instance)) {
            instance->recovery_failure_count++;
        }
        return;
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *handle)
{
    for (size_t i = 0U; i < usart_instance_count; ++i) {
        USARTInstance *instance = &usart_instances[i];

        if (instance->usart_handle != handle) {
            continue;
        }

        instance->error_count++;
        if (!USARTServiceInit(instance)) {
            instance->recovery_failure_count++;
        }
        return;
    }
}
