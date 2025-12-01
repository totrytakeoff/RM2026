/**
 * @file m3508.cpp
 * @brief M3508 类实现
 */

extern "C" {
#include "stm32f4xx_hal.h"
}

#include "m3508.hpp"
#include <cmath>

// M3508 编码器分辨率
static constexpr uint16_t ECD_RANGE = 8192;
static constexpr float ECD_TO_DEGREE = 360.0f / ECD_RANGE;

M3508::M3508(CanBus* can, uint8_t motor_id)
    : can_(can)
    , id_(motor_id)
    , slot_(0)
    , fbk_id_(0x200 + motor_id)
    , mode_(ControlMode::OPEN_LOOP)
    , target_speed_(0.0f)
    , target_position_(0.0f)
    , output_current_(0)
    , speed_pid_(10.0f, 0.5f, 0.0f)  // 默认速度环 PID 参数
    , pos_pid_(0.5f, 0.0f, 0.1f)     // 默认位置环 PID 参数
    , initialized_(false)
{
    // 确定槽位索引
    if (id_ >= 1 && id_ <= 4) {
        slot_ = id_ - 1;
    } else if (id_ >= 5 && id_ <= 8) {
        slot_ = id_ - 5;
    } else {
        slot_ = 0;
    }
    
    // 初始化测量数据
    meas_ = {0};
    
    // 配置 PID 参数
    speed_pid_.setOutputLimits(-16384.0f, 16384.0f);
    speed_pid_.setSampleTime(0.001f);  // 1ms
    speed_pid_.setMode(PIDController::Mode::AUTOMATIC);
    
    pos_pid_.setOutputLimits(-500.0f, 500.0f);  // 位置环输出限制为速度
    pos_pid_.setSampleTime(0.001f);
    pos_pid_.setMode(PIDController::Mode::AUTOMATIC);
}

bool M3508::init() {
    if (!can_ || initialized_) {
        return false;
    }
    
    // 注册 CAN 接收回调（带 ID 过滤）
    bool success = can_->registerRxCallback(canRxCallback, fbk_id_, false, this);
    if (success) {
        initialized_ = true;
    }
    
    return success;
}

bool M3508::setCurrent(int16_t current)
{
    if (!can_) return false;
    
    // 限幅
    if (current > 16384) current = 16384;
    if (current < -16384) current = -16384;
    
    output_current_ = current;
    
    uint8_t p[8] = {0};
    put16(p, slot_*2, current);
    
    // 根据 ID 选择组播帧
    uint16_t group_id = (id_ <= 4) ? kGroupCurrent : kGroupCurrent2;
    return can_->sendStd(group_id, p, 8) == HAL_OK;
}

void M3508::setTargetSpeed(float target_rpm) {
    target_speed_ = target_rpm;
    mode_ = ControlMode::SPEED_LOOP;
}

void M3508::setTargetPosition(float target_angle) {
    target_position_ = target_angle;
    mode_ = ControlMode::POSITION_LOOP;
}

bool M3508::update(float dt) {
    if (!can_) return false;
    
    int16_t current = 0;
    
    switch (mode_) {
        case ControlMode::OPEN_LOOP:
            // 开环模式，不做处理
            return true;
            
        case ControlMode::SPEED_LOOP: {
            // 速度环控制
            float current_speed = static_cast<float>(meas_.speed_rpm);
            float output = (dt > 0.0f) 
                ? speed_pid_.compute(target_speed_, current_speed, dt)
                : speed_pid_.compute(target_speed_, current_speed);
            current = static_cast<int16_t>(output);
            break;
        }
        
        case ControlMode::POSITION_LOOP: {
            // 位置环控制（级联）
            float current_pos = static_cast<float>(meas_.total_angle);
            
            // 位置环输出作为速度环输入
            float target_speed = (dt > 0.0f)
                ? pos_pid_.compute(target_position_, current_pos, dt)
                : pos_pid_.compute(target_position_, current_pos);
            
            // 速度环输出电流
            float current_speed = static_cast<float>(meas_.speed_rpm);
            float output = (dt > 0.0f)
                ? speed_pid_.compute(target_speed, current_speed, dt)
                : speed_pid_.compute(target_speed, current_speed);
            current = static_cast<int16_t>(output);
            break;
        }
    }
    
    return setCurrent(current);
}

void M3508::setControlMode(ControlMode mode) {
    if (mode_ != mode) {
        mode_ = mode;
        // 切换模式时重置 PID
        speed_pid_.reset();
        pos_pid_.reset();
    }
}

void M3508::setSpeedPID(float kp, float ki, float kd) {
    speed_pid_.setTunings(kp, ki, kd);
}

void M3508::setPositionPID(float kp, float ki, float kd) {
    pos_pid_.setTunings(kp, ki, kd);
}

void M3508::reset() {
    speed_pid_.reset();
    pos_pid_.reset();
    meas_.total_angle = 0;
    meas_.total_rounds = 0;
    output_current_ = 0;
    target_speed_ = 0.0f;
    target_position_ = 0.0f;
}

bool M3508::sendCurrentGroup(CanBus* can, int16_t i1, int16_t i2, int16_t i3, int16_t i4)
{
    if (!can) return false;
    uint8_t p[8]; 
    pack4(p, i1, i2, i3, i4);
    return can->sendStd(kGroupCurrent, p, 8) == HAL_OK;
}

bool M3508::sendCurrentGroup2(CanBus* can, int16_t i1, int16_t i2, int16_t i3, int16_t i4)
{
    if (!can) return false;
    uint8_t p[8]; 
    pack4(p, i1, i2, i3, i4);
    return can->sendStd(kGroupCurrent2, p, 8) == HAL_OK;
}

bool M3508::updateSpeedGroup(M3508* m1, M3508* m2, M3508* m3, M3508* m4, float dt)
{
    if (!m1 || !m2 || !m3 || !m4) return false;
    
    // 计算各电机的电流输出
    int16_t currents[4] = {0};
    
    if (m1->mode_ == ControlMode::SPEED_LOOP) {
        float speed = static_cast<float>(m1->meas_.speed_rpm);
        float output = (dt > 0.0f) 
            ? m1->speed_pid_.compute(m1->target_speed_, speed, dt)
            : m1->speed_pid_.compute(m1->target_speed_, speed);
        currents[0] = static_cast<int16_t>(output);
    }
    
    if (m2->mode_ == ControlMode::SPEED_LOOP) {
        float speed = static_cast<float>(m2->meas_.speed_rpm);
        float output = (dt > 0.0f)
            ? m2->speed_pid_.compute(m2->target_speed_, speed, dt)
            : m2->speed_pid_.compute(m2->target_speed_, speed);
        currents[1] = static_cast<int16_t>(output);
    }
    
    if (m3->mode_ == ControlMode::SPEED_LOOP) {
        float speed = static_cast<float>(m3->meas_.speed_rpm);
        float output = (dt > 0.0f)
            ? m3->speed_pid_.compute(m3->target_speed_, speed, dt)
            : m3->speed_pid_.compute(m3->target_speed_, speed);
        currents[2] = static_cast<int16_t>(output);
    }
    
    if (m4->mode_ == ControlMode::SPEED_LOOP) {
        float speed = static_cast<float>(m4->meas_.speed_rpm);
        float output = (dt > 0.0f)
            ? m4->speed_pid_.compute(m4->target_speed_, speed, dt)
            : m4->speed_pid_.compute(m4->target_speed_, speed);
        currents[3] = static_cast<int16_t>(output);
    }
    
    // 批量发送
    return sendCurrentGroup(m1->can_, currents[0], currents[1], currents[2], currents[3]);
}

bool M3508::updatePositionGroup(M3508* m1, M3508* m2, M3508* m3, M3508* m4, float dt)
{
    if (!m1 || !m2 || !m3 || !m4) return false;
    
    // 计算各电机的电流输出
    int16_t currents[4] = {0};
    
    if (m1->mode_ == ControlMode::POSITION_LOOP) {
        float pos = static_cast<float>(m1->meas_.total_angle);
        float target_speed = (dt > 0.0f)
            ? m1->pos_pid_.compute(m1->target_position_, pos, dt)
            : m1->pos_pid_.compute(m1->target_position_, pos);
        float speed = static_cast<float>(m1->meas_.speed_rpm);
        float output = (dt > 0.0f)
            ? m1->speed_pid_.compute(target_speed, speed, dt)
            : m1->speed_pid_.compute(target_speed, speed);
        currents[0] = static_cast<int16_t>(output);
    }
    
    if (m2->mode_ == ControlMode::POSITION_LOOP) {
        float pos = static_cast<float>(m2->meas_.total_angle);
        float target_speed = (dt > 0.0f)
            ? m2->pos_pid_.compute(m2->target_position_, pos, dt)
            : m2->pos_pid_.compute(m2->target_position_, pos);
        float speed = static_cast<float>(m2->meas_.speed_rpm);
        float output = (dt > 0.0f)
            ? m2->speed_pid_.compute(target_speed, speed, dt)
            : m2->speed_pid_.compute(target_speed, speed);
        currents[1] = static_cast<int16_t>(output);
    }
    
    if (m3->mode_ == ControlMode::POSITION_LOOP) {
        float pos = static_cast<float>(m3->meas_.total_angle);
        float target_speed = (dt > 0.0f)
            ? m3->pos_pid_.compute(m3->target_position_, pos, dt)
            : m3->pos_pid_.compute(m3->target_position_, pos);
        float speed = static_cast<float>(m3->meas_.speed_rpm);
        float output = (dt > 0.0f)
            ? m3->speed_pid_.compute(target_speed, speed, dt)
            : m3->speed_pid_.compute(target_speed, speed);
        currents[2] = static_cast<int16_t>(output);
    }
    
    if (m4->mode_ == ControlMode::POSITION_LOOP) {
        float pos = static_cast<float>(m4->meas_.total_angle);
        float target_speed = (dt > 0.0f)
            ? m4->pos_pid_.compute(m4->target_position_, pos, dt)
            : m4->pos_pid_.compute(m4->target_position_, pos);
        float speed = static_cast<float>(m4->meas_.speed_rpm);
        float output = (dt > 0.0f)
            ? m4->speed_pid_.compute(target_speed, speed, dt)
            : m4->speed_pid_.compute(target_speed, speed);
        currents[3] = static_cast<int16_t>(output);
    }
    
    // 批量发送
    return sendCurrentGroup(m1->can_, currents[0], currents[1], currents[2], currents[3]);
}

void M3508::setSpeedGroup(M3508* m1, M3508* m2, M3508* m3, M3508* m4,
                          float s1, float s2, float s3, float s4)
{
    if (m1) m1->setTargetSpeed(s1);
    if (m2) m2->setTargetSpeed(s2);
    if (m3) m3->setTargetSpeed(s3);
    if (m4) m4->setTargetSpeed(s4);
}

void M3508::setPositionGroup(M3508* m1, M3508* m2, M3508* m3, M3508* m4,
                             float p1, float p2, float p3, float p4)
{
    if (m1) m1->setTargetPosition(p1);
    if (m2) m2->setTargetPosition(p2);
    if (m3) m3->setTargetPosition(p3);
    if (m4) m4->setTargetPosition(p4);
}

bool M3508::controlSpeedDirect(CanBus* can,
                               float target1, float target2, float target3, float target4,
                               float current1, float current2, float current3, float current4,
                               float kp, float ki, float kd, uint8_t group)
{
    if (!can) return false;
    
    // 简单 PID 计算（仅 P 控制，适合快速测试）
    // 如需完整 PID，建议创建电机实例
    int16_t currents[4];
    
    float error1 = target1 - current1;
    float error2 = target2 - current2;
    float error3 = target3 - current3;
    float error4 = target4 - current4;
    
    currents[0] = static_cast<int16_t>(error1 * kp);
    currents[1] = static_cast<int16_t>(error2 * kp);
    currents[2] = static_cast<int16_t>(error3 * kp);
    currents[3] = static_cast<int16_t>(error4 * kp);
    
    // 限幅
    for (int i = 0; i < 4; i++) {
        if (currents[i] > 16384) currents[i] = 16384;
        if (currents[i] < -16384) currents[i] = -16384;
    }
    
    // 发送
    uint16_t can_id = (group == 1) ? kGroupCurrent : kGroupCurrent2;
    uint8_t data[8];
    pack4(data, currents[0], currents[1], currents[2], currents[3]);
    return can->sendStd(can_id, data, 8) == HAL_OK;
}

bool M3508::controlPositionDirect(CanBus* can,
                                  float target_pos1, float target_pos2, float target_pos3, float target_pos4,
                                  float current_pos1, float current_pos2, float current_pos3, float current_pos4,
                                  float current_spd1, float current_spd2, float current_spd3, float current_spd4,
                                  float pos_kp, float pos_ki, float pos_kd,
                                  float spd_kp, float spd_ki, float spd_kd,
                                  uint8_t group)
{
    if (!can) return false;
    
    // 位置环计算（简化 PD 控制）
    float target_speeds[4];
    target_speeds[0] = (target_pos1 - current_pos1) * pos_kp;
    target_speeds[1] = (target_pos2 - current_pos2) * pos_kp;
    target_speeds[2] = (target_pos3 - current_pos3) * pos_kp;
    target_speeds[3] = (target_pos4 - current_pos4) * pos_kp;
    
    // 限制速度环输出
    for (int i = 0; i < 4; i++) {
        if (target_speeds[i] > 500.0f) target_speeds[i] = 500.0f;
        if (target_speeds[i] < -500.0f) target_speeds[i] = -500.0f;
    }
    
    // 速度环计算（简化 P 控制）
    int16_t currents[4];
    currents[0] = static_cast<int16_t>((target_speeds[0] - current_spd1) * spd_kp);
    currents[1] = static_cast<int16_t>((target_speeds[1] - current_spd2) * spd_kp);
    currents[2] = static_cast<int16_t>((target_speeds[2] - current_spd3) * spd_kp);
    currents[3] = static_cast<int16_t>((target_speeds[3] - current_spd4) * spd_kp);
    
    // 限幅
    for (int i = 0; i < 4; i++) {
        if (currents[i] > 16384) currents[i] = 16384;
        if (currents[i] < -16384) currents[i] = -16384;
    }
    
    // 发送
    uint16_t can_id = (group == 1) ? kGroupCurrent : kGroupCurrent2;
    uint8_t data[8];
    pack4(data, currents[0], currents[1], currents[2], currents[3]);
    return can->sendStd(can_id, data, 8) == HAL_OK;
}

void M3508::parseFeedback(const CAN_RxHeaderTypeDef* h, const uint8_t* d)
{
    if (!h || !d) return;
    if (h->IDE != CAN_ID_STD) return;
    if (h->StdId != fbk_id_) return;
    
    meas_.last_ecd = meas_.ecd;
    meas_.ecd = (uint16_t)((d[0] << 8) | d[1]);
    meas_.speed_rpm = (int16_t)((d[2] << 8) | d[3]);
    meas_.given_current = (int16_t)((d[4] << 8) | d[5]);
    meas_.temperature = d[6];
    
    // 更新累计角度
    updateTotalAngle();
}

void M3508::canRxCallback(const CAN_RxHeaderTypeDef* header, const uint8_t* data, void* user) {
    if (!user) return;
    M3508* motor = static_cast<M3508*>(user);
    motor->parseFeedback(header, data);
}

void M3508::updateTotalAngle() {
    if (meas_.last_ecd == 0 && meas_.ecd == 0) {
        return;  // 首次接收，不计算
    }
    
    int32_t delta = static_cast<int32_t>(meas_.ecd) - static_cast<int32_t>(meas_.last_ecd);
    
    // 处理编码器溢出
    if (delta > ECD_RANGE / 2) {
        delta -= ECD_RANGE;
        meas_.total_rounds--;
    } else if (delta < -ECD_RANGE / 2) {
        delta += ECD_RANGE;
        meas_.total_rounds++;
    }
    
    // 计算累计角度（度）
    meas_.total_angle = meas_.total_rounds * 360 + static_cast<int32_t>(meas_.ecd * ECD_TO_DEGREE);
}

const M3508::Measure& M3508::measure() const { 
    return meas_; 
}

uint8_t M3508::id() const { 
    return id_; 
}

uint16_t M3508::feedbackId() const { 
    return fbk_id_; 
}

uint8_t M3508::slot() const { 
    return slot_; 
}

M3508::ControlMode M3508::getControlMode() const {
    return mode_;
}

float M3508::getCurrentSpeed() const {
    return static_cast<float>(meas_.speed_rpm);
}

float M3508::getCurrentPosition() const {
    return static_cast<float>(meas_.total_angle);
}

int16_t M3508::getLastCurrent() const {
    return output_current_;
}

void M3508::put16(uint8_t* p, uint8_t off, int16_t v)
{
    p[off] = (uint8_t)(v >> 8); 
    p[off+1] = (uint8_t)(v);
}

void M3508::pack4(uint8_t* p, int16_t a, int16_t b, int16_t c, int16_t d)
{
    put16(p, 0, a); 
    put16(p, 2, b); 
    put16(p, 4, c); 
    put16(p, 6, d);
}
