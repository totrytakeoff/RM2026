#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>

#include "main.h"

#define USART_MAX_INSTANCES 3U
#define USART_RX_BUFFER_CAPACITY 256U
#define USART_BLOCKING_TX_TIMEOUT_MS 100U

typedef void (*USARTReceiveCallback)(void);

typedef enum {
    USART_TRANSFER_NONE = 0,
    USART_TRANSFER_BLOCKING,
    USART_TRANSFER_IT,
    USART_TRANSFER_DMA,
} USART_TRANSFER_MODE;

typedef struct {
    uint8_t recv_buff[USART_RX_BUFFER_CAPACITY];
    uint16_t recv_buff_size;
    uint16_t recv_len;
    UART_HandleTypeDef *usart_handle;
    USARTReceiveCallback module_callback;
    /** Debugger-visible counters written from the UART interrupt path. */
    volatile uint32_t rx_event_count;
    volatile uint32_t error_count;
    volatile uint32_t recovery_failure_count;
} USARTInstance;

typedef struct {
    uint16_t recv_buff_size;
    UART_HandleTypeDef *usart_handle;
    USARTReceiveCallback module_callback;
} USART_Init_Config_s;

/** Register an exclusive UART endpoint in fixed static storage during startup. */
USARTInstance *USARTRegister(const USART_Init_Config_s *config);

/** Restart receive-to-idle DMA for an endpoint. */
uint8_t USARTServiceInit(USARTInstance *instance);

/** Send one frame using the selected transfer mode. */
uint8_t USARTSend(USARTInstance *instance,
                  uint8_t *send_buf,
                  uint16_t send_size,
                  USART_TRANSFER_MODE mode);

/** Return 1 when the UART transmit state is ready. */
uint8_t USARTIsReady(const USARTInstance *instance);

#endif /* BSP_USART_H */
