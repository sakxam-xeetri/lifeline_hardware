#include "mpu_manager.h"
#include <math.h>

MPUManager mpuManager;

MPUManager::MPUManager() : _mpu(Wire), _buf_idx(0), _vibe_sample_count(0) {
    _data = {0, 0, 0, 0, 0, 0, 1.0f, 0.0f, false, false, false, false, false};
    for (int i = 0; i < MPU_MOVING_AVG_WINDOW; i++) {
        _accel_buf[i] = 1.0f;
    }
}

void MPUManager::begin() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    byte status = _mpu.begin();
    #if SPU_DEBUG_ENABLE
    Serial.printf("[MPU] MPU6050 init status: %d\n", status);
    #endif
    if (status == 0) {
        delay(100);
        _mpu.calcOffsets(true, true); // Auto-calibrate gyro and accel offset
        #if SPU_DEBUG_ENABLE
        Serial.println(F("[MPU] Offsets calculated successfully."));
        #endif
    }
}

void MPUManager::update() {
    _mpu.update();

    _data.accel_x = _mpu.getAccX();
    _data.accel_y = _mpu.getAccY();
    _data.accel_z = _mpu.getAccZ();

    _data.gyro_x = _mpu.getGyroX();
    _data.gyro_y = _mpu.getGyroY();
    _data.gyro_z = _mpu.getGyroZ();

    // Magnitude of acceleration vector
    float raw_mag = sqrtf(_data.accel_x * _data.accel_x +
                          _data.accel_y * _data.accel_y +
                          _data.accel_z * _data.accel_z);

    // Apply moving average filter
    _data.total_accel_g = getFilteredAccel(raw_mag);

    // Calculate Tilt Angle relative to vertical Z axis
    if (_data.total_accel_g > 0.1f) {
        float cos_tilt = _data.accel_z / _data.total_accel_g;
        cos_tilt = constrain(cos_tilt, -1.0f, 1.0f);
        _data.tilt_deg = acosf(cos_tilt) * (180.0f / M_PI);
    } else {
        _data.tilt_deg = 0.0f;
    }

    // Motion Detection Rules
    _data.is_motion_detected = fabs(_data.total_accel_g - 1.0f) > 0.15f;
    _data.is_free_fall = (_data.total_accel_g < THRESHOLD_FREE_FALL_G);
    _data.is_sudden_impact = (raw_mag > THRESHOLD_IMPACT_G);
    _data.is_excessive_tilt = (_data.tilt_deg > THRESHOLD_TILT_DEG);

    // Seismic / Continuous Vibration Detection
    if (fabs(raw_mag - 1.0f) > (THRESHOLD_VIBE_EARTHQ - 1.0f)) {
        _vibe_sample_count++;
        if (_vibe_sample_count >= EARTHQUAKE_VIBE_WINDOW) {
            _data.is_earthquake_vibration = true;
        }
    } else {
        if (_vibe_sample_count > 0) _vibe_sample_count--;
        if (_vibe_sample_count < EARTHQUAKE_VIBE_WINDOW / 2) {
            _data.is_earthquake_vibration = false;
        }
    }
}

float MPUManager::getFilteredAccel(float raw_accel) {
    _accel_buf[_buf_idx] = raw_accel;
    _buf_idx = (_buf_idx + 1) % MPU_MOVING_AVG_WINDOW;

    float sum = 0.0f;
    for (int i = 0; i < MPU_MOVING_AVG_WINDOW; i++) {
        sum += _accel_buf[i];
    }
    return sum / (float)MPU_MOVING_AVG_WINDOW;
}
