#include "health_calculator.h"
#include <algorithm>

SystemHealthMetrics HealthCalculator::calculate(const EnvironmentData& env,
                                                const MotionData& motion,
                                                const GasData& gas,
                                                const GPSData& gps) {
    SystemHealthMetrics metrics;

    // 1. Calculate Node Health Score (100% ideal)
    int health = 100;
    if (!env.valid) health -= 15;
    if (!gps.fix_valid) health -= 10;
    
    // Battery degradation penalty
    if (env.battery_percent < 50) {
        health -= (50 - env.battery_percent) / 2;
    }
    metrics.node_health_score = (uint8_t)std::max(0, std::min(100, health));

    // 2. Calculate Environmental Risk Score (0% ideal, 100% maximum danger)
    int risk = 0;

    // Gas Risk Contribution (0-40 points)
    if (gas.ppm_estimate > 100) {
        int gas_points = (gas.ppm_estimate - 100) / 15;
        risk += std::min(40, gas_points);
    }

    // Temperature Anomaly Contribution (0-30 points)
    if (env.temperature_c > 35.0f) {
        risk += (int)((env.temperature_c - 35.0f) * 2.5f);
    } else if (env.temperature_c < 5.0f) {
        risk += (int)((5.0f - env.temperature_c) * 2.0f);
    }

    // Seismic / Motion Contribution (0-30 points)
    if (motion.is_earthquake_vibration) {
        risk += 30;
    } else if (motion.is_sudden_impact || motion.is_excessive_tilt) {
        risk += 20;
    } else if (motion.is_motion_detected) {
        risk += 5;
    }

    metrics.environmental_risk_score = (uint8_t)std::max(0, std::min(100, risk));

    return metrics;
}
