#include "shoot.h"
#include "robot_def.h"

#include "dji_motor.h"
#include "message_center.h"
#include "bsp_dwt.h"
#include "general_def.h"

/* 对于双发射机构的机器人,将下面的数据封装成结构体即可,生成两份shoot应用实例 */
static DJIMotorInstance *friction_l, *friction_r, *loader; // 拨盘电机
// static servo_instance *lid; 需要增加弹舱盖

static Publisher_t *shoot_pub;
static Shoot_Ctrl_Cmd_s shoot_cmd_recv; // 来自cmd的发射控制信息
static Subscriber_t *shoot_sub;
static Shoot_Upload_Data_s shoot_feedback_data; // 来自cmd的发射控制信息

// dwt定时,计算冷却用
static float hibernate_time = 0, dead_time = 0;
static loader_mode_e last_load_mode = LOAD_STOP;
static float single_shot_target = 0.0f;
static float loader_speed_cmd = 0.0f;
static float loader_last_update_ms = 0.0f;

static float ShootGetFrictionTargetSpeed(Bullet_Speed_e speed)
{
    switch (speed)
    {
    case SMALL_AMU_15:
        return SHOOT_FRICTION_SPEED_15;
    case SMALL_AMU_18:
        return SHOOT_FRICTION_SPEED_18;
    case SMALL_AMU_30:
        return SHOOT_FRICTION_SPEED_30;
    default:
        return SHOOT_FRICTION_SPEED_DEFAULT;
    }
}

void ShootInit()
{
    // 左摩擦轮
    Motor_Init_Config_s friction_config = {
        .can_init_config = {
            .can_handle = &SHOOT_FRICTION_CAN_HANDLE,
        },
        .controller_param_init_config = {
            .speed_PID = {
                .Kp = SHOOT_FRICTION_SPEED_PID_KP,
                .Ki = SHOOT_FRICTION_SPEED_PID_KI,
                .Kd = SHOOT_FRICTION_SPEED_PID_KD,
                .Improve = SHOOT_FRICTION_SPEED_PID_IMPROVE,
                .IntegralLimit = SHOOT_FRICTION_SPEED_PID_INTEGRAL_LIMIT,
                .MaxOut = SHOOT_FRICTION_SPEED_PID_MAX_OUT,
            },
            .current_PID = {
                .Kp = SHOOT_FRICTION_CURRENT_PID_KP,
                .Ki = SHOOT_FRICTION_CURRENT_PID_KI,
                .Kd = SHOOT_FRICTION_CURRENT_PID_KD,
                .Improve = SHOOT_FRICTION_CURRENT_PID_IMPROVE,
                .IntegralLimit = SHOOT_FRICTION_CURRENT_PID_INTEGRAL_LIMIT,
                .MaxOut = SHOOT_FRICTION_CURRENT_PID_MAX_OUT,
            },
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED,
            .speed_feedback_source = MOTOR_FEED,

            .outer_loop_type = SHOOT_FRICTION_INIT_OUTER_LOOP,
            .close_loop_type = SHOOT_FRICTION_INIT_CLOSE_LOOP,
            .motor_reverse_flag = SHOOT_FRICTION_LEFT_DIR,
        },
        .motor_type = SHOOT_FRICTION_MOTOR_TYPE};
    friction_config.can_init_config.tx_id = SHOOT_FRICTION_LEFT_MOTOR_ID;
    friction_l = DJIMotorInit(&friction_config);

    friction_config.can_init_config.tx_id = SHOOT_FRICTION_RIGHT_MOTOR_ID; // 右摩擦轮,改txid和方向就行
    friction_config.controller_setting_init_config.motor_reverse_flag = SHOOT_FRICTION_RIGHT_DIR;
    friction_r = DJIMotorInit(&friction_config);

    // 拨盘电机
    Motor_Init_Config_s loader_config = {
        .can_init_config = {
            .can_handle = &SHOOT_LOADER_CAN_HANDLE,
            .tx_id = SHOOT_LOADER_MOTOR_ID,
        },
        .controller_param_init_config = {
            .angle_PID = {
                // 如果启用位置环来控制发弹,需要较大的I值保证输出力矩的线性度否则出现接近拨出的力矩大幅下降
                .Kp = SHOOT_LOADER_ANGLE_PID_KP,
                .Ki = SHOOT_LOADER_ANGLE_PID_KI,
                .Kd = SHOOT_LOADER_ANGLE_PID_KD,
                .Improve = SHOOT_LOADER_ANGLE_PID_IMPROVE,
                .MaxOut = SHOOT_LOADER_ANGLE_PID_MAX_OUT,
            },
            .speed_PID = {
                .Kp = SHOOT_LOADER_SPEED_PID_KP,
                .Ki = SHOOT_LOADER_SPEED_PID_KI,
                .Kd = SHOOT_LOADER_SPEED_PID_KD,
                .Improve = SHOOT_LOADER_SPEED_PID_IMPROVE,
                .IntegralLimit = SHOOT_LOADER_SPEED_PID_INTEGRAL_LIMIT,
                .MaxOut = SHOOT_LOADER_SPEED_PID_MAX_OUT,
            },
            .current_PID = {
                .Kp = SHOOT_LOADER_CURRENT_PID_KP,
                .Ki = SHOOT_LOADER_CURRENT_PID_KI,
                .Kd = SHOOT_LOADER_CURRENT_PID_KD,
                .Improve = SHOOT_LOADER_CURRENT_PID_IMPROVE,
                .IntegralLimit = SHOOT_LOADER_CURRENT_PID_INTEGRAL_LIMIT,
                .MaxOut = SHOOT_LOADER_CURRENT_PID_MAX_OUT,
            },
        },
        .controller_setting_init_config = {
            .angle_feedback_source = MOTOR_FEED, .speed_feedback_source = MOTOR_FEED,
            .outer_loop_type = SHOOT_LOADER_INIT_OUTER_LOOP, // 初始化成SPEED_LOOP,让拨盘停在原地,防止拨盘上电时乱转
            .close_loop_type = SHOOT_LOADER_INIT_CLOSE_LOOP,
            .motor_reverse_flag = SHOOT_LOADER_DIR, // 注意方向设置为拨盘的拨出的击发方向
        },
        .motor_type = SHOOT_LOADER_MOTOR_TYPE // 英雄使用m3508
    };
    loader = DJIMotorInit(&loader_config);

    shoot_pub = PubRegister("shoot_feed", sizeof(Shoot_Upload_Data_s));
    shoot_sub = SubRegister("shoot_cmd", sizeof(Shoot_Ctrl_Cmd_s));
}

/* 机器人发射机构控制核心任务 */
void ShootTask()
{
    // 从cmd获取控制数据
    SubGetMessage(shoot_sub, &shoot_cmd_recv);
    float now = DWT_GetTimeline_ms();

    // 对shoot mode等于SHOOT_STOP的情况特殊处理,直接停止所有电机(紧急停止)
    if (shoot_cmd_recv.shoot_mode == SHOOT_OFF)
    {
        DJIMotorStop(friction_l);
        DJIMotorStop(friction_r);
        DJIMotorStop(loader);
        loader_speed_cmd = 0.0f;
        loader_last_update_ms = now;
    }
    else // 恢复运行
    {
        DJIMotorEnable(friction_l);
        DJIMotorEnable(friction_r);
        DJIMotorEnable(loader);
    }

    // 如果上一次触发单发或3发指令的时间加上不应期仍然大于当前时间(尚未休眠完毕),直接返回即可
    // 单发模式主要提供给能量机关激活使用(以及英雄的射击大部分处于单发)
    uint8_t loader_cooling_down = (hibernate_time + dead_time > now);

    // 若不在休眠状态,根据robotCMD传来的控制模式进行拨盘电机参考值设定和模式切换
    if (loader_cooling_down &&
        (shoot_cmd_recv.load_mode == LOAD_1_BULLET || shoot_cmd_recv.load_mode == LOAD_3_BULLET))
    {
        DJIMotorOuterLoop(loader, SHOOT_LOADER_LOOP_COOLING);
        DJIMotorSetRef(loader, single_shot_target);
    }
    else switch (shoot_cmd_recv.load_mode)
    {
    // 停止拨盘
    case LOAD_STOP:
        DJIMotorOuterLoop(loader, SHOOT_LOADER_LOOP_STOP); // 切换到速度环
        DJIMotorSetRef(loader, SHOOT_LOADER_STOP_REF); // 同时设定参考值为0,这样停止的速度最快
        loader_speed_cmd = 0.0f;
        loader_last_update_ms = now;
        break;
    // 单发模式,根据鼠标按下的时间,触发一次之后需要进入不响应输入的状态(否则按下的时间内可能多次进入,导致多次发射)
    case LOAD_1_BULLET:                                                               // 激活能量机关干扰对方英雄用
        if (last_load_mode != LOAD_1_BULLET)
        {
            single_shot_target = loader->measure.total_angle + ONE_BULLET_DELTA_ANGLE;
            hibernate_time = now;
            dead_time = SHOOT_LOADER_SINGLE_DEADTIME_MS;
        }
        DJIMotorOuterLoop(loader, SHOOT_LOADER_LOOP_SINGLE);                          // 切换到角度环
        DJIMotorSetRef(loader, single_shot_target);                                   // 控制量增加一发弹丸的角度
        break;
    case LOAD_3_BULLET:
        if (last_load_mode != LOAD_3_BULLET)
        {
            single_shot_target = loader->measure.total_angle + 3.0f * ONE_BULLET_DELTA_ANGLE;
            hibernate_time = now;
            dead_time = SHOOT_LOADER_THREE_DEADTIME_MS;
        }
        DJIMotorOuterLoop(loader, SHOOT_LOADER_LOOP_THREE);                                    // 切换到速度环
        DJIMotorSetRef(loader, single_shot_target);
        loader_speed_cmd = 0.0f;
        loader_last_update_ms = now;
        break;
    // 连发模式,对速度闭环,射频后续修改为可变当前固定1Hz
    case LOAD_BURSTFIRE:
    {
        float speed_target;
        float dt_ms;
        float max_delta;
        DJIMotorOuterLoop(loader, SHOOT_LOADER_LOOP_BURST);
        speed_target = shoot_cmd_recv.shoot_rate * 360.0f * REDUCTION_RATIO_LOADER / NUM_PER_CIRCLE;
        if (last_load_mode != LOAD_BURSTFIRE)
        {
            loader_speed_cmd = 0.0f;
            loader_last_update_ms = now;
        }

        dt_ms = now - loader_last_update_ms;
        if (dt_ms < 0.0f)
            dt_ms = 0.0f;
        loader_last_update_ms = now;
        max_delta = SHOOT_LOADER_CONTINUOUS_SLEW_PER_MS * dt_ms;
        if (speed_target > loader_speed_cmd + max_delta)
            loader_speed_cmd += max_delta;
        else if (speed_target < loader_speed_cmd - max_delta)
            loader_speed_cmd -= max_delta;
        else
            loader_speed_cmd = speed_target;
        DJIMotorSetRef(loader, loader_speed_cmd);
        // x颗/秒换算成速度: 已知一圈的载弹量,由此计算出1s需要转的角度,注意换算角速度(DJIMotor的速度单位是angle per second)
        break;
    }
    // 拨盘反转,对速度闭环,后续增加卡弹检测(通过裁判系统剩余热量反馈和电机电流)
    // 也有可能需要从switch-case中独立出来
    case LOAD_REVERSE:
        DJIMotorOuterLoop(loader, SHOOT_LOADER_LOOP_REVERSE);
        loader_speed_cmd = 0.0f;
        loader_last_update_ms = now;
        // ...
        break;
    default:
        while (1)
            ; // 未知模式,停止运行,检查指针越界,内存溢出等问题
    }

    last_load_mode = shoot_cmd_recv.load_mode;

    // 确定是否开启摩擦轮,后续可能修改为键鼠模式下始终开启摩擦轮(上场时建议一直开启)
    if (shoot_cmd_recv.friction_mode == FRICTION_ON)
    {
        float friction_target = ShootGetFrictionTargetSpeed(shoot_cmd_recv.bullet_speed);
        DJIMotorSetRef(friction_l, friction_target);
        DJIMotorSetRef(friction_r, friction_target);
    }
    else // 关闭摩擦轮
    {
        DJIMotorSetRef(friction_l, SHOOT_FRICTION_STOP_REF);
        DJIMotorSetRef(friction_r, SHOOT_FRICTION_STOP_REF);
    }

    // 开关弹舱盖
    if (shoot_cmd_recv.lid_mode == LID_CLOSE)
    {
        //...
    }
    else if (shoot_cmd_recv.lid_mode == LID_OPEN)
    {
        //...
    }

    // 反馈数据,目前暂时没有要设定的反馈数据,后续可能增加应用离线监测以及卡弹反馈
    PubPushMessage(shoot_pub, (void *)&shoot_feedback_data);
}
