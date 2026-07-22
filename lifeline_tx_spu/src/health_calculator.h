#ifndef HEALTH_CALCULATOR_H
#define HEALTH_CALCULATOR_H

#include <Arduino.h>
#include "environment_manager.h"
#include "mpu_manager.h"
#include "gas_manager.h"
#include "gps_manager.h"

struct SystemHealthMetrics {
    uint8_t node_health_score;      // 0 - 100%
    uint8_t environmental_risk_score; // 0 - 100%
};

class HealthCalculator {
public:
    static SystemHealthMetrics calculate(const EnvironmentData& env,
                                          const MotionData& motion,
                                          const GasData& gas,
                                          const GPSData& gps);
};

#endif // HEALTH_CALCULATOR_H
