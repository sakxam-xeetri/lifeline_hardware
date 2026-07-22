#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "ConfigSPU.h"
#include "SharedProtocol.h"
#include "gps_manager.h"
#include "environment_manager.h"
#include "mpu_manager.h"
#include "gas_manager.h"
#include "emergency_detector.h"
#include "health_calculator.h"
#include "uart_manager.h"

class SensorManager {
public:
    SensorManager();
    void begin();
    void loop();

private:
    unsigned long _last_sample_time;
    unsigned long _last_send_time;

    TelemetryPacket buildTelemetryPacket(const EnvironmentData& env,
                                         const MotionData& motion,
                                         const GasData& gas,
                                         const GPSData& gps,
                                         const EmergencyState& emergency,
                                         const SystemHealthMetrics& health);
};

extern SensorManager sensorManager;

#endif // SENSOR_MANAGER_H
