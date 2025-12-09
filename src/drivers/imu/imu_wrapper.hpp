#pragma once

#include "BMI088driver.h"
#include "ist8310driver.h"
#include "common/utils.hpp"

class IMUWrapper {
public:
    IMUWrapper();
    ~IMUWrapper() = default;

    bool init();
    void read_data(float accel[3], float gyro[3], float mag[3], float* temperature = nullptr);

private:
    bool bmi088_initialized_{false};
    bool ist8310_initialized_{false};

    // Calibration offsets
    float accel_offset_[3] = {0};
    float gyro_offset_[3] = {0};
    float mag_offset_[3] = {0};

    void calibrate_sensors();
};