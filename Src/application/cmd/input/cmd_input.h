#ifndef CMD_INPUT_H
#define CMD_INPUT_H

#include "stdint.h"
#include "robot_def.h"

typedef struct
{
    const Chassis_Upload_Data_s *chassis_feedback;
    const Gimbal_Upload_Data_s *gimbal_feedback;
    const Shoot_Upload_Data_s *shoot_feedback;
} RobotCMDInput_Context_s;

typedef struct
{
    uint8_t input_online;     // 当前输入源是否在线
    uint8_t request_stop;     // 输入源请求急停
    uint8_t request_recover;  // 输入源允许恢复

    Chassis_Ctrl_Cmd_s chassis_cmd;
    Gimbal_Ctrl_Cmd_s gimbal_cmd;
    Shoot_Ctrl_Cmd_s shoot_cmd;
} RobotCMDInput_Data_s;

/**
 * @brief 初始化当前选中的输入后端
 */
void RobotCMDInputInit(void);

/**
 * @brief 更新输入并生成统一控制命令
 */
void RobotCMDInputUpdate(const RobotCMDInput_Context_s *ctx, RobotCMDInput_Data_s *data);

#endif // CMD_INPUT_H
