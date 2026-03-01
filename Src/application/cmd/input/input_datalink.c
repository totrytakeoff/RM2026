#include "cmd_input.h"

void RobotCMDInputDatalinkInit(void)
{
    // 预留: 后续接入图传链路控制初始化
}

void RobotCMDInputDatalinkUpdate(const RobotCMDInput_Context_s *ctx, RobotCMDInput_Data_s *data)
{
    (void)ctx;

    if (data == NULL)
        return;

    // 当前未接入图传,默认保持急停请求
    data->request_stop = 1u;
}
