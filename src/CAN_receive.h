#ifndef CAN_RECEIVE_H
#define CAN_RECEIVE_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHASSIS_CAN hcan1
#define GIMBAL_CAN  hcan1

// rm motor data
typedef struct
{
    uint16_t ecd;
    int16_t  speed_rpm;
    int16_t  given_current;
    uint8_t  temperate;
    int16_t  last_ecd;
} motor_measure_t;

// CAN IDs
typedef enum
{
    CAN_CHASSIS_ALL_ID   = 0x200,
    CAN_3508_M1_ID       = 0x201,
    CAN_3508_M2_ID       = 0x202,
    CAN_3508_M3_ID       = 0x203,
    CAN_3508_M4_ID       = 0x204,

    CAN_YAW_MOTOR_ID     = 0x205,
    CAN_PIT_MOTOR_ID     = 0x206,
    CAN_TRIGGER_MOTOR_ID = 0x207,
    CAN_GIMBAL_ALL_ID    = 0x1FF,
} can_msg_id_e;

void CAN_cmd_gimbal(int16_t yaw, int16_t pitch, int16_t shoot, int16_t rev);
void CAN_cmd_chassis(int16_t motor1, int16_t motor2, int16_t motor3, int16_t motor4);
void CAN_cmd_chassis_reset_ID(void);

const motor_measure_t *get_yaw_gimbal_motor_measure_point(void);
const motor_measure_t *get_pitch_gimbal_motor_measure_point(void);
const motor_measure_t *get_trigger_motor_measure_point(void);
const motor_measure_t *get_chassis_motor_measure_point(uint8_t i);

#ifdef __cplusplus
}
#endif

#endif // CAN_RECEIVE_H
