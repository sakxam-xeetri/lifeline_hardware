#include "emergency_detector.h"

EmergencyDetector emergencyDetector;

EmergencyDetector::EmergencyDetector() {
    _state = {EMERGENCY_NONE, PRIORITY_NORMAL, "NORMAL", false};
}

void EmergencyDetector::begin() {
    #if SPU_DEBUG_ENABLE
    Serial.println(F("[EMERGENCY] Emergency Fusion Detector Initialized."));
    #endif
}

void EmergencyDetector::update(const EnvironmentData& env, const MotionData& motion, const GasData& gas) {
    // 1. SENSOR FUSION: Forest Fire / High Gas Fire Danger
    if (gas.is_gas_danger && env.temperature_c >= THRESHOLD_FIRE_TEMP) {
        _state.code = EMERGENCY_FIRE;
        _state.priority = PRIORITY_CRITICAL;
        _state.description = "POSSIBLE FIRE / GAS EXPLOSION";
        _state.is_active = true;
        return;
    }

    // 2. SENSOR FUSION: Earthquake (Continuous High Vibration)
    if (motion.is_earthquake_vibration) {
        _state.code = EMERGENCY_EARTHQUAKE;
        _state.priority = PRIORITY_CRITICAL;
        _state.description = "EARTHQUAKE / SEISMIC VIBRATION";
        _state.is_active = true;
        return;
    }

    // 3. SENSOR FUSION: Landslide (Excessive Tilt + Sudden Impact / Motion)
    if (motion.is_excessive_tilt && (motion.is_sudden_impact || motion.is_motion_detected)) {
        _state.code = EMERGENCY_LANDSLIDE;
        _state.priority = PRIORITY_HIGH;
        _state.description = "POSSIBLE LANDSLIDE / COLLAPSE";
        _state.is_active = true;
        return;
    }

    // 4. Gas Leak / Smoke Warning
    if (gas.is_gas_danger) {
        _state.code = EMERGENCY_GAS;
        _state.priority = PRIORITY_HIGH;
        _state.description = "HAZARDOUS GAS LEAK / SMOKE";
        _state.is_active = true;
        return;
    }

    // 5. Environmental Heat Wave
    if (env.temperature_c >= THRESHOLD_HEAT_TEMP) {
        _state.code = EMERGENCY_HEAT;
        _state.priority = PRIORITY_MEDIUM;
        _state.description = "EXTREME HEAT WAVE ALERT";
        _state.is_active = true;
        return;
    }

    // 6. Environmental Cold Wave
    if (env.temperature_c <= THRESHOLD_COLD_TEMP) {
        _state.code = EMERGENCY_COLD;
        _state.priority = PRIORITY_MEDIUM;
        _state.description = "EXTREME COLD / FREEZING ALERT";
        _state.is_active = true;
        return;
    }

    // 7. High Humidity Warning
    if (env.humidity_pct >= THRESHOLD_HIGH_HUMIDITY) {
        _state.code = EMERGENCY_HUMIDITY;
        _state.priority = PRIORITY_LOW;
        _state.description = "HIGH HUMIDITY WARNING";
        _state.is_active = true;
        return;
    }

    // If no emergency conditions present
    _state.code = EMERGENCY_NONE;
    _state.priority = PRIORITY_NORMAL;
    _state.description = "NORMAL TELEMETRY";
    _state.is_active = false;
}
