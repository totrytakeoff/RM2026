#ifndef INFANTRY_INPUT_H
#define INFANTRY_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "infantry_types.h"
#include "remote_control_state.h"

#ifdef __cplusplus
extern "C" {
#endif

bool Input_Init(void);
void Input_GetData(Input_Data_t *data);
uint8_t Input_IsOnline(void);
RemoteControlType Input_GetRemoteType(void);
bool Input_GetRemoteState(RemoteControlState *state);

#ifdef __cplusplus
}
#endif

#endif /* INFANTRY_INPUT_H */
