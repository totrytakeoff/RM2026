#include "dji_motor.h"
#include "general_def.h"
#include "bsp_dwt.h"
#include "bsp_log.h"
#include "rm_critical.h"
#include <math.h>
#include <string.h>

static uint8_t idx = 0; // register idx,是该文件的全局电机索引,在注册时使用
/* Fixed-capacity storage for registered DJI motor instances. */
static DJIMotorInstance dji_motor_storage[DJI_MOTOR_CNT];
static DJIMotorInstance *dji_motor_instance[DJI_MOTOR_CNT] = {NULL}; // 会在control任务中遍历该指针数组进行pid计算

/**
 * DJI ESC commands share one frame per group of four motors. These six
 * transmit-only instances cover three command groups on each CAN bus.
 *
 * C610(m2006)/C620(m3508):0x1ff,0x200;
 * GM6020:0x1ff,0x2ff
 * 反馈(rx_id): GM6020: 0x204+id ; C610/C620: 0x200+id
 * can1: [0]:0x1FF,[1]:0x200,[2]:0x2FF
 * can2: [3]:0x1FF,[4]:0x200,[5]:0x2FF
 */
static CANInstance sender_assignment[6] = {
    [0] = {.can_handle = &hcan1, .txconf.StdId = 0x1ff, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
    [1] = {.can_handle = &hcan1, .txconf.StdId = 0x200, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
    [2] = {.can_handle = &hcan1, .txconf.StdId = 0x2ff, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
    [3] = {.can_handle = &hcan2, .txconf.StdId = 0x1ff, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
    [4] = {.can_handle = &hcan2, .txconf.StdId = 0x200, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
    [5] = {.can_handle = &hcan2, .txconf.StdId = 0x2ff, .txconf.IDE = CAN_ID_STD, .txconf.RTR = CAN_RTR_DATA, .txconf.DLC = 0x08, .tx_buff = {0}},
};

/** Enable only command groups containing a successfully registered motor. */
static uint8_t sender_enable_flag[6] = {0};

/**
 * @brief 根据电调/拨码开关上的ID,根据说明书的默认id分配方式计算发送ID和接收ID,
 *        并对电机进行分组以便处理多电机控制命令。
 */
static bool MotorSenderGrouping(DJIMotorInstance *motor,
                                CAN_Init_Config_s *config)
{
    uint8_t motor_id;
    uint8_t motor_send_num;
    uint8_t motor_grouping;

    if ((motor == NULL) || (config == NULL) ||
        ((config->can_handle != &hcan1) &&
         (config->can_handle != &hcan2)) ||
        (config->tx_id < 1U) || (config->tx_id > 8U)) {
        return false;
    }
    motor_id = (uint8_t)(config->tx_id - 1U);

    switch (motor->motor_type)
    {
    case M2006:
    case M3508:
        if (motor_id < 4) // 根据ID分组
        {
            motor_send_num = motor_id;
            motor_grouping = config->can_handle == &hcan1 ? 1 : 4;
        }
        else
        {
            motor_send_num = motor_id - 4;
            motor_grouping = config->can_handle == &hcan1 ? 0 : 3;
        }

        // 计算接收id并设置分组发送id
        config->rx_id = 0x200 + motor_id + 1;   // 把ID+1,进行分组设置
        motor->message_num = motor_send_num;
        motor->sender_group = motor_grouping;

        // 检查是否发生id冲突
        for (size_t i = 0; i < idx; ++i)
        {
            if (dji_motor_instance[i]->motor_can_instance->can_handle == config->can_handle && dji_motor_instance[i]->motor_can_instance->rx_id == config->rx_id)
            {
                LOGERROR("[dji_motor] ID crash. Check in debug mode, add dji_motor_instance to watch to get more information.");
                uint16_t can_bus = config->can_handle == &hcan1 ? 1 : 2;
                LOGERROR("[dji_motor] id [%lu], can_bus [%u]",
                         (unsigned long)config->rx_id,
                         (unsigned)can_bus);
                return false;
            }
        }
        break;

    case GM6020:
        if (motor_id < 4)
        {
            motor_send_num = motor_id;
            motor_grouping = config->can_handle == &hcan1 ? 0 : 3;
        }
        else
        {
            motor_send_num = motor_id - 4;
            motor_grouping = config->can_handle == &hcan1 ? 2 : 5;
        }

        config->rx_id = 0x204 + motor_id + 1;   // 把ID+1,进行分组设置
        motor->message_num = motor_send_num;
        motor->sender_group = motor_grouping;

        for (size_t i = 0; i < idx; ++i)
        {
            if (dji_motor_instance[i]->motor_can_instance->can_handle == config->can_handle && dji_motor_instance[i]->motor_can_instance->rx_id == config->rx_id)
            {
                LOGERROR("[dji_motor] ID crash. Check in debug mode, add dji_motor_instance to watch to get more information.");
                uint16_t can_bus = config->can_handle == &hcan1 ? 1 : 2;
                LOGERROR("[dji_motor] id [%lu], can_bus [%u]",
                         (unsigned long)config->rx_id,
                         (unsigned)can_bus);
                return false;
            }
        }
        break;

    default:
        LOGERROR("[dji_motor] unsupported motor type");
        return false;
    }

    return true;
}

/**
 * @todo 评估是否可以简化多圈角度计算。
 * @brief 解码 CAN 反馈并发布一致性测量快照。
 *
 * @param can_instance 收到反馈的 CAN 端点。
 */
bool DJIMotorGetMeasure(const DJIMotorInstance *motor,
                        DJI_Motor_Measure_s *measure)
{
    RmCriticalState state;

    if ((motor == NULL) || (measure == NULL)) {
        return false;
    }

    state = RmCritical_Enter();
    memcpy(measure, &motor->measure, sizeof(*measure));
    RmCritical_Exit(state);
    return true;
}

static void DecodeDJIMotor(CANInstance *can_instance)
{
    const uint8_t *rx_buffer;
    DJIMotorInstance *motor;
    DJI_Motor_Measure_s next;
    bool first_feedback;
    RmCriticalState state;

    if ((can_instance == NULL) || (can_instance->id == NULL) ||
        (can_instance->rx_len < 7U)) {
        return;
    }

    rx_buffer = can_instance->rx_buff;
    motor = (DJIMotorInstance *)can_instance->id;
    if (!DJIMotorGetMeasure(motor, &next)) {
        return;
    }
    first_feedback = (motor->feedback_initialized == 0U);

    motor->dt = DWT_GetDeltaT(&motor->feed_cnt);

    next.last_ecd = next.ecd;
    next.ecd = ((uint16_t)rx_buffer[0] << 8U) | rx_buffer[1];
    next.angle_single_round = ECD_ANGLE_COEF_DJI * (float)next.ecd;
    next.speed_aps =
        (1.0f - SPEED_SMOOTH_COEF) * next.speed_aps +
        RPM_2_ANGLE_PER_SEC * SPEED_SMOOTH_COEF *
            (float)((int16_t)((uint16_t)rx_buffer[2] << 8U |
                              rx_buffer[3]));
    next.real_current =
        (int16_t)((1.0f - CURRENT_SMOOTH_COEF) *
                      (float)next.real_current +
                  CURRENT_SMOOTH_COEF *
                      (float)((int16_t)((uint16_t)rx_buffer[4] << 8U |
                                        rx_buffer[5])));
    next.temperature = rx_buffer[6];

    if (first_feedback) {
        next.last_ecd = next.ecd;
    } else {
        if ((int32_t)next.ecd - (int32_t)next.last_ecd > 4096) {
            next.total_round--;
        } else if ((int32_t)next.ecd - (int32_t)next.last_ecd < -4096) {
            next.total_round++;
        }
    }
    next.total_angle = (float)next.total_round * 360.0f +
                       next.angle_single_round;

    state = RmCritical_Enter();
    memcpy(&motor->measure, &next, sizeof(next));
    motor->feedback_initialized = 1U;
    RmCritical_Exit(state);
    DaemonReload(motor->daemon);
}

static void DJIMotorLostCallback(void *motor_ptr)
{
    DJIMotorInstance *motor = (DJIMotorInstance *)motor_ptr;

    if ((motor == NULL) || (motor->motor_can_instance == NULL)) {
        return;
    }
    uint16_t can_bus = motor->motor_can_instance->can_handle == &hcan1 ? 1 : 2;
    LOGWARNING("[dji_motor] Motor lost, can bus [%d] , id [%d]", can_bus, motor->motor_can_instance->tx_id);
}

// 初始化并返回一个固定存储的电机实例。
DJIMotorInstance *DJIMotorInit(Motor_Init_Config_s *config)
{
    if ((config == NULL) || (idx >= DJI_MOTOR_CNT))
    {
        LOGERROR("[dji_motor] motor instance capacity exhausted");
        return NULL;
    }

    DJIMotorInstance *instance = &dji_motor_storage[idx];
    memset(instance, 0, sizeof(DJIMotorInstance));

    // motor basic setting 电机基本设置
    instance->motor_type = config->motor_type;                         // 6020 or 2006 or 3508
    instance->motor_settings = config->controller_setting_init_config;

    // motor controller init 电机控制器初始化
    PIDInit(&instance->motor_controller.current_PID, &config->controller_param_init_config.current_PID);
    PIDInit(&instance->motor_controller.speed_PID, &config->controller_param_init_config.speed_PID);
    PIDInit(&instance->motor_controller.angle_PID, &config->controller_param_init_config.angle_PID);
    instance->motor_controller.other_angle_feedback_ptr = config->controller_param_init_config.other_angle_feedback_ptr;
    instance->motor_controller.other_speed_feedback_ptr = config->controller_param_init_config.other_speed_feedback_ptr;
    instance->motor_controller.current_feedforward_ptr = config->controller_param_init_config.current_feedforward_ptr;
    instance->motor_controller.speed_feedforward_ptr = config->controller_param_init_config.speed_feedforward_ptr;
    // 前馈源由应用配置，控制任务只读取对应指针。

    // 电机分组,因为至多4个电机可以共用一帧CAN控制报文
    if (!MotorSenderGrouping(instance, &config->can_init_config)) {
        LOGERROR("[dji_motor] invalid motor CAN grouping");
        return NULL;
    }

    // 注册电机到CAN总线
    config->can_init_config.can_module_callback = DecodeDJIMotor; // set callback
    config->can_init_config.id = instance;                        // set id,eq to address(it is identity)
    instance->motor_can_instance = CANRegister(&config->can_init_config);
    if (instance->motor_can_instance == NULL)
    {
        LOGERROR("[dji_motor] CAN endpoint registration failed");
        return NULL;
    }

    // 注册守护线程
    DaemonConfig daemon_config = {
        .callback = DJIMotorLostCallback,
        .owner = instance,
        .timeout_ms = 20U, // 20ms未收到数据则丢失
    };
    instance->daemon = DaemonRegister(&daemon_config);
    if (instance->daemon == NULL)
    {
        LOGERROR("[dji_motor] health endpoint registration failed");
        /* Keep the CAN callback owner reserved, but never enable output. */
        DJIMotorStop(instance);
        dji_motor_instance[idx++] = instance;
        return NULL;
    }

    instance->last_total_angle = 0.0f;
    instance->angle_feedback_sign = 1;
    instance->angle_feedback_locked = 0;

    DJIMotorEnable(instance);
    sender_enable_flag[instance->sender_group] = 1U;
    dji_motor_instance[idx++] = instance;
    return instance;
}

/* 电流反馈来自电调；外部力矩传感器应作为独立反馈源接入。 */
void DJIMotorChangeFeed(DJIMotorInstance *motor, Closeloop_Type_e loop, Feedback_Source_e type)
{
    if (motor == NULL) {
        return;
    }
    if (loop == ANGLE_LOOP)
        motor->motor_settings.angle_feedback_source = type;
    else if (loop == SPEED_LOOP)
        motor->motor_settings.speed_feedback_source = type;
    else
        LOGERROR("[dji_motor] loop type error, check memory access and func param"); // 检查是否传入了正确的LOOP类型,或发生了指针越界
}

void DJIMotorStop(DJIMotorInstance *motor)
{
    if (motor != NULL) {
        motor->stop_flag = MOTOR_STOP;
    }
}

void DJIMotorEnable(DJIMotorInstance *motor)
{
    if (motor != NULL) {
        motor->stop_flag = MOTOR_ENALBED;
    }
}

/* 修改电机的最外层闭环。 */
void DJIMotorOuterLoop(DJIMotorInstance *motor, Closeloop_Type_e outer_loop)
{
    if (motor != NULL) {
        motor->motor_settings.outer_loop_type = outer_loop;
    }
}

// 设置控制参考值。
void DJIMotorSetRef(DJIMotorInstance *motor, float ref)
{
    if (motor != NULL) {
        motor->motor_controller.pid_ref = ref;
    }
}

// 为所有电机实例计算串级 PID，并发送分组控制报文。
void DJIMotorControl()
{
    // 保存局部引用，避免在控制计算中重复索引实例表。
    uint8_t group, num; // 电机组号和组内编号
    int16_t set;        // CAN 电流控制值
    DJIMotorInstance *motor;
    Motor_Control_Setting_s *motor_setting; // 电机控制参数
    Motor_Controller_s *motor_controller;   // 电机控制器
    DJI_Motor_Measure_s measure_snapshot;
    const DJI_Motor_Measure_s *measure;
    float pid_measure, pid_ref;             // PID 测量值和参考值

    // 遍历所有电机实例，计算串级 PID 并填充发送报文。
    for (size_t i = 0; i < idx; ++i)
    {
        motor = dji_motor_instance[i];
        motor_setting = &motor->motor_settings;
        motor_controller = &motor->motor_controller;
        if (!DJIMotorGetMeasure(motor, &measure_snapshot)) {
            continue;
        }
        measure = &measure_snapshot;
        pid_ref = motor_controller->pid_ref;
        if (motor_setting->motor_reverse_flag == MOTOR_DIRECTION_REVERSE)
            pid_ref *= -1;

        float delta_angle = measure->total_angle - motor->last_total_angle;
        if (!motor->angle_feedback_locked)
        {
            if (fabsf(measure->speed_aps) > 5.0f && fabsf(delta_angle) > 0.1f)
            {
                motor->angle_feedback_sign = (measure->speed_aps * delta_angle >= 0.0f) ? 1 : -1;
                motor->angle_feedback_locked = 1;
            }
        }
        // pid_ref 依次流过已启用的外环。
        // 仅在位置环为最外环时计算位置环输出。
        if ((motor_setting->close_loop_type & ANGLE_LOOP) && motor_setting->outer_loop_type == ANGLE_LOOP)
        {
            if (motor_setting->angle_feedback_source == OTHER_FEED)
                pid_measure = (motor_controller->other_angle_feedback_ptr != NULL)
                                  ? *motor_controller->other_angle_feedback_ptr
                                  : measure->total_angle;
            else
                pid_measure = measure->total_angle * motor->angle_feedback_sign; // MOTOR_FEED,对total angle闭环,防止在边界处出现突跃
            // 更新pid_ref进入下一个环
            pid_ref = PIDCalculate(&motor_controller->angle_PID, pid_measure, pid_ref);
        }

        // 当速度环处于有效控制链中时计算速度环。
        if ((motor_setting->close_loop_type & SPEED_LOOP) && (motor_setting->outer_loop_type & (ANGLE_LOOP | SPEED_LOOP)))
        {
            if (motor_setting->feedforward_flag & SPEED_FEEDFORWARD)
                pid_ref += (motor_controller->speed_feedforward_ptr != NULL)
                               ? *motor_controller->speed_feedforward_ptr
                               : 0.0f;

            if (motor_setting->speed_feedback_source == OTHER_FEED)
                pid_measure = (motor_controller->other_speed_feedback_ptr != NULL)
                                  ? *motor_controller->other_speed_feedback_ptr
                                  : measure->speed_aps;
            else // MOTOR_FEED
                pid_measure = measure->speed_aps;
            // 更新pid_ref进入下一个环
            pid_ref = PIDCalculate(&motor_controller->speed_PID, pid_measure, pid_ref);
        }

        // 电流环使用电调反馈，位于控制链最内层。
        if (motor_setting->feedforward_flag & CURRENT_FEEDFORWARD)
            pid_ref += (motor_controller->current_feedforward_ptr != NULL)
                           ? *motor_controller->current_feedforward_ptr
                           : 0.0f;
        if (motor_setting->close_loop_type & CURRENT_LOOP)
        {
            pid_ref = PIDCalculate(&motor_controller->current_PID, measure->real_current, pid_ref);
        }

        if (motor_setting->feedback_reverse_flag == FEEDBACK_DIRECTION_REVERSE)
            pid_ref *= -1;

        // 获取最终输出。
        set = (int16_t)pid_ref;

        // 按电机组填入发送数据。
        group = motor->sender_group;
        num = motor->message_num;
        sender_assignment[group].tx_buff[2 * num] = (uint8_t)(set >> 8);
        sender_assignment[group].tx_buff[2 * num + 1] = (uint8_t)(set & 0x00ff);

        // 停止状态直接清零该电机对应的两个字节。
        if (motor->stop_flag == MOTOR_STOP)
            memset(sender_assignment[group].tx_buff + 2 * num, 0, 2u);

        motor->last_total_angle = measure->total_angle;
    }

    // 仅发送包含已注册电机的命令组。
    for (size_t i = 0; i < 6; ++i)
    {
        if (sender_enable_flag[i])
        {
            CANTransmit(&sender_assignment[i], 1000U);
        }
    }
}
