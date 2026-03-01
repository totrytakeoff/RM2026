#include "cmd_input.h"

#include "string.h"

void RobotCMDInputET08Init(void);
void RobotCMDInputET08Update(const RobotCMDInput_Context_s *ctx, RobotCMDInput_Data_s *data);

void RobotCMDInputDT7Init(void);
void RobotCMDInputDT7Update(const RobotCMDInput_Context_s *ctx, RobotCMDInput_Data_s *data);

void RobotCMDInputDatalinkInit(void);
void RobotCMDInputDatalinkUpdate(const RobotCMDInput_Context_s *ctx, RobotCMDInput_Data_s *data);

static void RobotCMDInputSetSafeDefault(RobotCMDInput_Data_s *data)
{
    memset(data, 0, sizeof(*data));

    data->chassis_cmd.chassis_mode = CHASSIS_ZERO_FORCE;
    data->gimbal_cmd.gimbal_mode = GIMBAL_ZERO_FORCE;
    data->shoot_cmd.shoot_mode = SHOOT_OFF;
    data->shoot_cmd.load_mode = LOAD_STOP;
    data->shoot_cmd.friction_mode = FRICTION_OFF;
    data->shoot_cmd.lid_mode = CMD_DEFAULT_LID_MODE;
    data->shoot_cmd.bullet_speed = CMD_DEFAULT_BULLET_SPEED;
    data->shoot_cmd.shoot_rate = CMD_DEFAULT_SHOOT_RATE;
}

void RobotCMDInputInit(void)
{
#if ROBOT_CMD_INPUT_SOURCE == ROBOT_CMD_INPUT_SRC_ET08
    RobotCMDInputET08Init();
#elif ROBOT_CMD_INPUT_SOURCE == ROBOT_CMD_INPUT_SRC_DT7
    RobotCMDInputDT7Init();
#elif ROBOT_CMD_INPUT_SOURCE == ROBOT_CMD_INPUT_SRC_DATALINK
    RobotCMDInputDatalinkInit();
#else
#error Invalid ROBOT_CMD_INPUT_SOURCE
#endif
}

void RobotCMDInputUpdate(const RobotCMDInput_Context_s *ctx, RobotCMDInput_Data_s *data)
{
    if (data == NULL)
        return;

    RobotCMDInputSetSafeDefault(data);

#if ROBOT_CMD_INPUT_SOURCE == ROBOT_CMD_INPUT_SRC_ET08
    RobotCMDInputET08Update(ctx, data);
#elif ROBOT_CMD_INPUT_SOURCE == ROBOT_CMD_INPUT_SRC_DT7
    RobotCMDInputDT7Update(ctx, data);
#elif ROBOT_CMD_INPUT_SOURCE == ROBOT_CMD_INPUT_SRC_DATALINK
    RobotCMDInputDatalinkUpdate(ctx, data);
#else
#error Invalid ROBOT_CMD_INPUT_SOURCE
#endif
}
