#include "CAN_receive.h"
#include "can.h"
#include "main.h"

#define get_motor_measure(ptr, data)                  \
    do {                                              \
        (ptr)->last_ecd     = (ptr)->ecd;             \
        (ptr)->ecd          = (uint16_t)((data)[0] << 8 | (data)[1]); \
        (ptr)->speed_rpm    = (int16_t)((data)[2] << 8 | (data)[3]);  \
        (ptr)->given_current= (int16_t)((data)[4] << 8 | (data)[5]);  \
        (ptr)->temperate    = (data)[6];              \
    } while (0)

static motor_measure_t motor_chassis[7];

static CAN_TxHeaderTypeDef  gimbal_tx_message;
static uint8_t              gimbal_can_send_data[8];
static CAN_TxHeaderTypeDef  chassis_tx_message;
static uint8_t              chassis_can_send_data[8];

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) != HAL_OK)
    {
        return;
    }

    switch (rx_header.StdId)
    {
        case CAN_3508_M1_ID:
        case CAN_3508_M2_ID:
        case CAN_3508_M3_ID:
        case CAN_3508_M4_ID:
        case CAN_YAW_MOTOR_ID:
        case CAN_PIT_MOTOR_ID:
        case CAN_TRIGGER_MOTOR_ID:
        {
            uint8_t i = (uint8_t)(rx_header.StdId - CAN_3508_M1_ID);
            if (i < 7)
            {
                get_motor_measure(&motor_chassis[i], rx_data);
            }
            break;
        }
        default:
            break;
    }
}

void CAN_cmd_gimbal(int16_t yaw, int16_t pitch, int16_t shoot, int16_t rev)
{
    uint32_t send_mail_box;
    gimbal_tx_message.StdId = CAN_GIMBAL_ALL_ID;
    gimbal_tx_message.IDE = CAN_ID_STD;
    gimbal_tx_message.RTR = CAN_RTR_DATA;
    gimbal_tx_message.DLC = 0x08;
    gimbal_can_send_data[0] = (uint8_t)(yaw >> 8);
    gimbal_can_send_data[1] = (uint8_t)(yaw);
    gimbal_can_send_data[2] = (uint8_t)(pitch >> 8);
    gimbal_can_send_data[3] = (uint8_t)(pitch);
    gimbal_can_send_data[4] = (uint8_t)(shoot >> 8);
    gimbal_can_send_data[5] = (uint8_t)(shoot);
    gimbal_can_send_data[6] = (uint8_t)(rev >> 8);
    gimbal_can_send_data[7] = (uint8_t)(rev);
    HAL_CAN_AddTxMessage(&GIMBAL_CAN, &gimbal_tx_message, gimbal_can_send_data, &send_mail_box);
}

void CAN_cmd_chassis_reset_ID(void)
{
    uint32_t send_mail_box;
    chassis_tx_message.StdId = 0x700;
    chassis_tx_message.IDE = CAN_ID_STD;
    chassis_tx_message.RTR = CAN_RTR_DATA;
    chassis_tx_message.DLC = 0x08;
    for (int i = 0; i < 8; i++) chassis_can_send_data[i] = 0;
    HAL_CAN_AddTxMessage(&CHASSIS_CAN, &chassis_tx_message, chassis_can_send_data, &send_mail_box);
}

void CAN_cmd_chassis(int16_t motor1, int16_t motor2, int16_t motor3, int16_t motor4)
{
    uint32_t send_mail_box;
    chassis_tx_message.StdId = CAN_CHASSIS_ALL_ID;
    chassis_tx_message.IDE = CAN_ID_STD;
    chassis_tx_message.RTR = CAN_RTR_DATA;
    chassis_tx_message.DLC = 0x08;
    chassis_can_send_data[0] = (uint8_t)(motor1 >> 8);
    chassis_can_send_data[1] = (uint8_t)(motor1);
    chassis_can_send_data[2] = (uint8_t)(motor2 >> 8);
    chassis_can_send_data[3] = (uint8_t)(motor2);
    chassis_can_send_data[4] = (uint8_t)(motor3 >> 8);
    chassis_can_send_data[5] = (uint8_t)(motor3);
    chassis_can_send_data[6] = (uint8_t)(motor4 >> 8);
    chassis_can_send_data[7] = (uint8_t)(motor4);
    HAL_CAN_AddTxMessage(&CHASSIS_CAN, &chassis_tx_message, chassis_can_send_data, &send_mail_box);
}

const motor_measure_t *get_yaw_gimbal_motor_measure_point(void)
{
    return &motor_chassis[4];
}

const motor_measure_t *get_pitch_gimbal_motor_measure_point(void)
{
    return &motor_chassis[5];
}

const motor_measure_t *get_trigger_motor_measure_point(void)
{
    return &motor_chassis[6];
}

const motor_measure_t *get_chassis_motor_measure_point(uint8_t i)
{
    return &motor_chassis[(i & 0x03)];
}
