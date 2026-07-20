#ifndef INFANTRY_CHASSIS_KINEMATICS_H
#define INFANTRY_CHASSIS_KINEMATICS_H

#ifdef __cplusplus
extern "C" {
#endif

/** 将云台坐标系平移指令旋转到底盘坐标系。 */
void InfantryChassis_RotateToBody(float gimbal_vx,
                                  float gimbal_vy,
                                  float yaw_offset_rad,
                                  float *body_vx,
                                  float *body_vy);

/** 按向量等比例限制平移速度，避免对角输入超过执行层总速度。 */
void InfantryChassis_LimitTranslation(float *vx,
                                      float *vy,
                                      float max_speed);

/** 当前四全向轮布置的逆运动学，输出轮端 rad/s。 */
void InfantryChassis_OmniInverse(float vx,
                                 float vy,
                                 float wz,
                                 float wheel_base,
                                 float wheel_radius,
                                 float output[4]);

/** 将轮端 rad/s 按减速比换算为电机转子 rad/s。 */
float InfantryChassis_WheelToMotorSpeedRadS(float wheel_speed_rad_s,
                                            float reduction_ratio);

/** 四轮共同等比例缩放，保持合成运动方向不被逐轮削顶破坏。 */
void InfantryChassis_NormalizeWheelSpeeds(float speeds[4],
                                          float max_abs_speed);

#ifdef __cplusplus
}
#endif

#endif /* INFANTRY_CHASSIS_KINEMATICS_H */
