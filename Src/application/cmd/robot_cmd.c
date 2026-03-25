// app
#include "robot_def.h"
#include "robot_cmd.h"
#include "input/cmd_input.h"
// module
#include "master_process.h"
#include "message_center.h"
#include "dji_motor.h"
// bsp
#include "bsp_log.h"
#include "usart.h"

// 私有宏,自动将编码器转换成角度值
#define YAW_ALIGN_ANGLE (YAW_CHASSIS_ALIGN_ECD * ECD_ANGLE_COEF_DJI)  // 对齐时的角度,0-360
#define PTICH_HORIZON_ANGLE (PITCH_HORIZON_ECD * ECD_ANGLE_COEF_DJI)  // pitch水平时电机的角度,0-360

/* cmd应用包含的模块实例指针和交互信息存储*/
#ifdef GIMBAL_BOARD // 对双板的兼容,条件编译
#include "can_comm.h"
static CANCommInstance *cmd_can_comm; // 双板通信
#endif
#ifdef ONE_BOARD
static Publisher_t *chassis_cmd_pub;    // 底盘控制消息发布者
static Subscriber_t *chassis_feed_sub;  // 底盘反馈信息订阅者
#endif // ONE_BOARD

static Chassis_Ctrl_Cmd_s chassis_cmd_send;       // 发送给底盘应用的信息,包括控制信息和UI绘制相关
static Chassis_Upload_Data_s chassis_fetch_data;  // 从底盘应用接收的反馈信息

static Vision_Recv_s *vision_recv_data __attribute__((unused));  // 视觉接收数据指针,初始化时返回

static Publisher_t *gimbal_cmd_pub;              // 云台控制消息发布者
static Subscriber_t *gimbal_feed_sub;            // 云台反馈信息订阅者
static Gimbal_Ctrl_Cmd_s gimbal_cmd_send;        // 传递给云台的控制信息
static Gimbal_Upload_Data_s gimbal_fetch_data;   // 从云台获取的反馈信息

static Publisher_t *shoot_cmd_pub;               // 发射控制消息发布者
static Subscriber_t *shoot_feed_sub;             // 发射反馈信息订阅者
static Shoot_Ctrl_Cmd_s shoot_cmd_send;          // 传递给发射的控制信息
static Shoot_Upload_Data_s shoot_fetch_data;     // 从发射获取的反馈信息

static Robot_Status_e robot_state;               // 机器人整体工作状态

/**
 * @brief 根据gimbal app传回的当前电机角度计算和零位的误差
 *        单圈绝对角度的范围是0~360
 */
static void CalcOffsetAngle()
{
    static float angle;
    angle = gimbal_fetch_data.yaw_motor_single_round_angle;  // 从云台获取的当前yaw电机单圈角度
#if YAW_ECD_GREATER_THAN_4096
    if (angle > YAW_ALIGN_ANGLE && angle <= 180.0f + YAW_ALIGN_ANGLE)
        chassis_cmd_send.offset_angle = angle - YAW_ALIGN_ANGLE;
    else if (angle > 180.0f + YAW_ALIGN_ANGLE)
        chassis_cmd_send.offset_angle = angle - YAW_ALIGN_ANGLE - 360.0f;
    else
        chassis_cmd_send.offset_angle = angle - YAW_ALIGN_ANGLE;
#else
    if (angle > YAW_ALIGN_ANGLE)
        chassis_cmd_send.offset_angle = angle - YAW_ALIGN_ANGLE;
    else if (angle <= YAW_ALIGN_ANGLE && angle >= YAW_ALIGN_ANGLE - 180.0f)
        chassis_cmd_send.offset_angle = angle - YAW_ALIGN_ANGLE;
    else
        chassis_cmd_send.offset_angle = angle - YAW_ALIGN_ANGLE + 360.0f;
#endif
}

static void ForceStopCommand(void)
{
    gimbal_cmd_send.gimbal_mode = GIMBAL_ZERO_FORCE;
    chassis_cmd_send.chassis_mode = CHASSIS_ZERO_FORCE;
    shoot_cmd_send.shoot_mode = SHOOT_OFF;
    shoot_cmd_send.friction_mode = FRICTION_OFF;
    shoot_cmd_send.load_mode = LOAD_STOP;
}

/**
 * @brief 紧急停止处理:
 * 1) 当前输入源请求急停
 * 2) 已处于ROBOT_STOP
 * 恢复条件:
 * 1) 当前输入源明确请求恢复
 */
static void EmergencyHandler(const RobotCMDInput_Data_s *input_data)
{
    uint8_t need_stop = (input_data == NULL) || input_data->request_stop || (robot_state == ROBOT_STOP);

    if (need_stop)
    {
        if (robot_state != ROBOT_STOP)
            LOGERROR("[CMD] emergency stop!");
        robot_state = ROBOT_STOP;
        ForceStopCommand();
        return;
    }

    if (robot_state == ROBOT_STOP && input_data->request_recover)
    {
        robot_state = ROBOT_READY;
        shoot_cmd_send.shoot_mode = SHOOT_ON;
        LOGINFO("[CMD] reinstate, robot ready");
    }
}

void RobotCMDInit()
{
    // 输入源初始化与视觉接口初始化
    RobotCMDInputInit();
#ifdef VISION_ENABLE
    vision_recv_data = VisionInit(&ROBOT_CMD_UART_VISION_HANDLE);
#else
    // 视觉未启用,避免串口冲突 (需要开启时请在robot_def.h中定义 VISION_ENABLE)
#endif

    gimbal_cmd_pub = PubRegister("gimbal_cmd", sizeof(Gimbal_Ctrl_Cmd_s));
    gimbal_feed_sub = SubRegister("gimbal_feed", sizeof(Gimbal_Upload_Data_s));
    shoot_cmd_pub = PubRegister("shoot_cmd", sizeof(Shoot_Ctrl_Cmd_s));
    shoot_feed_sub = SubRegister("shoot_feed", sizeof(Shoot_Upload_Data_s));

#ifdef ONE_BOARD
    chassis_cmd_pub = PubRegister("chassis_cmd", sizeof(Chassis_Ctrl_Cmd_s));
    chassis_feed_sub = SubRegister("chassis_feed", sizeof(Chassis_Upload_Data_s));
#endif
#ifdef GIMBAL_BOARD
    CANComm_Init_Config_s comm_conf = {
        .can_config = {
            .can_handle = &ROBOT_CMD_CANCOMM_CAN_HANDLE,
            .tx_id = ROBOT_CMD_CANCOMM_TX_ID,
            .rx_id = ROBOT_CMD_CANCOMM_RX_ID,
        },
        .recv_data_len = sizeof(Chassis_Upload_Data_s),
        .send_data_len = sizeof(Chassis_Ctrl_Cmd_s),
    };
    cmd_can_comm = CANCommInit(&comm_conf);
#endif

    gimbal_cmd_send.pitch = 0.0f;
    robot_state = ROBOT_READY;
}

/* 机器人核心控制任务,200Hz频率运行(必须高于视觉发送频率) */
void RobotCMDTask()
{
    RobotCMDInput_Context_s input_ctx;
    RobotCMDInput_Data_s input_data;

    // 从其他应用获取回传数据
#ifdef ONE_BOARD
    SubGetMessage(chassis_feed_sub, (void *)&chassis_fetch_data);
#endif
#ifdef GIMBAL_BOARD
    chassis_fetch_data = *(Chassis_Upload_Data_s *)CANCommGet(cmd_can_comm);
#endif
    SubGetMessage(shoot_feed_sub, &shoot_fetch_data);
    SubGetMessage(gimbal_feed_sub, &gimbal_fetch_data);

    input_ctx.chassis_feedback = &chassis_fetch_data;
    input_ctx.gimbal_feedback = &gimbal_fetch_data;
    input_ctx.shoot_feedback = &shoot_fetch_data;

    RobotCMDInputUpdate(&input_ctx, &input_data);
    chassis_cmd_send = input_data.chassis_cmd;
    gimbal_cmd_send = input_data.gimbal_cmd;
    shoot_cmd_send = input_data.shoot_cmd;

    // 根据gimbal的反馈值计算云台和底盘正方向的夹角
    CalcOffsetAngle();
    EmergencyHandler(&input_data);

    // 推送消息,双板通信,视觉通信等
#ifdef ONE_BOARD
    PubPushMessage(chassis_cmd_pub, (void *)&chassis_cmd_send);
#endif
#ifdef GIMBAL_BOARD
    CANCommSend(cmd_can_comm, (void *)&chassis_cmd_send);
#endif
    PubPushMessage(shoot_cmd_pub, (void *)&shoot_cmd_send);
    PubPushMessage(gimbal_cmd_pub, (void *)&gimbal_cmd_send);
}
