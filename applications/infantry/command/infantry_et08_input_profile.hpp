#ifndef INFANTRY_ET08_INPUT_PROFILE_HPP
#define INFANTRY_ET08_INPUT_PROFILE_HPP

#include <cstdint>

#include "infantry_types.h"
#include "remote_control_state.h"

namespace rm {
namespace infantry {

/** ET08 设备校准参数；严禁加入任何机器人物理速度或执行器限位。 */
struct Et08InputProfileConfig {
    int16_t stick_deadzone_raw;
    float stick_full_scale_raw;
};

/**
 * 将通用 ET08 输入映射为步兵操作意图。
 *
 * 返回 false 表示快照不完整或无效；此时 output 已被置为急停安全值。
 */
bool MapEt08Input(const RemoteControlState &remote,
                  const Et08InputProfileConfig &config,
                  Input_Data_t &output);

} // namespace infantry
} // namespace rm

#endif /* INFANTRY_ET08_INPUT_PROFILE_HPP */
