#ifndef EMERGENCY_DETECTOR_H
#define EMERGENCY_DETECTOR_H

#include <Arduino.h>
#include "ConfigSPU.h"
#include "SharedProtocol.h"
#include "gps_manager.h"
#include "environment_manager.h"
#include "mpu_manager.h"
#include "gas_manager.h"

struct EmergencyState {
    uint8_t code;        // Emergency ASCII code (e.g. 'F', 'Q', 'L', 'S', etc.)
    uint8_t priority;    // 1 (Normal) to 5 (SOS/Critical)
    const char* description;
    bool is_active;
};

class EmergencyDetector {
public:
    EmergencyDetector();
    void begin();
    void update(const EnvironmentData& env, const MotionData& motion, const GasData& gas);

    const EmergencyState& getState() const { return _state; }

private:
    EmergencyState _state;
};

extern EmergencyDetector emergencyDetector;

#endif // EMERGENCY_DETECTOR_H
