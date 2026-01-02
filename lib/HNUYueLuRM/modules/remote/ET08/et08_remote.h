#ifndef ET08_REMOTE_H
#define ET08_REMOTE_H

#include <stdint.h>
#include "main.h"

#define ET08_CHANNEL_COUNT 8
#define ET08_CHANNEL_CENTER 1024
#define ET08_SWITCH_LEVEL_COUNT 6

// Negative levels are treated as centered values (raw - 1024) when mapping.
#define ET08_SWITCH_LEVEL_0 1896
#define ET08_SWITCH_LEVEL_1 1694
#define ET08_SWITCH_LEVEL_2 1493
#define ET08_SWITCH_LEVEL_3 -151
#define ET08_SWITCH_LEVEL_4 -353
#define ET08_SWITCH_LEVEL_5 -554

#define ET08_SWITCH_TOLERANCE 40

typedef enum
{
    ET08_CH1 = 0,
    ET08_CH2 = 1,
    ET08_CH3 = 2,
    ET08_CH4 = 3,
    ET08_CH5 = 4,
    ET08_CH6 = 5,
    ET08_CH7 = 6,
    ET08_CH8 = 7,
} ET08_Channel_t;

typedef struct
{
    int16_t x;
    int16_t y;
} ET08_Stick_t;

typedef struct
{
    uint16_t raw[ET08_CHANNEL_COUNT];
    int16_t centered[ET08_CHANNEL_COUNT];

    ET08_Stick_t right;
    ET08_Stick_t left;

    int16_t knob_left;
    int16_t knob_right;

    uint16_t switch_sa_sb_raw;
    int16_t switch_sa_sb_centered;
    uint8_t switch_sa_sb_state;

    uint16_t switch_sd_sc_raw;
    int16_t switch_sd_sc_centered;
    uint8_t switch_sd_sc_state;

    uint8_t frame_lost;
    uint8_t failsafe;
} ET08_Ctrl_t;

ET08_Ctrl_t *ET08_Init(UART_HandleTypeDef *uart_handle);
uint8_t ET08_IsOnline(void);
ET08_Ctrl_t *ET08_GetCtrl(void);
uint8_t ET08_MapSwitchState(uint16_t raw_value);

#endif
