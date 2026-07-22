#ifndef MPU_MANAGER_H
#define MPU_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include "ConfigSPU.h"

struct MotionData {
    float accel_x, accel_y, accel_z; // in G
    float gyro_x, gyro_y, gyro_z;    // in deg/sec
    float total_accel_g;              // Magnitude = sqrt(x^2 + y^2 + z^2)
    float tilt_deg;                   // Tilt relative to vertical
    
    bool is_motion_detected;
    bool is_free_fall;
    bool is_sudden_impact;
    bool is_excessive_tilt;
    bool is_earthquake_vibration;
};

class MPUManager {
public:
    MPUManager();
    void begin();
    void update();
    const MotionData& getData() const { return _data; }

private:
    MPU6050 _mpu;
    MotionData _data;
    
    float _accel_buf[MPU_MOVING_AVG_WINDOW];
    uint8_t _buf_idx;
    uint8_t _vibe_sample_count;

    float getFilteredAccel(float raw_accel);
};

extern MPUManager mpuManager;

#endif // MPU_MANAGER_H
