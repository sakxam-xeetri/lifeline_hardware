#include "sensor_manager.h"

SensorManager sensorManager;

SensorManager::SensorManager() : _last_sample_time(0), _last_send_time(0) {}

void SensorManager::begin() {
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    gpsManager.begin();
    envManager.begin();
    mpuManager.begin();
    gasManager.begin();
    emergencyDetector.begin();
    uartManager.begin();

    #if SPU_DEBUG_ENABLE
    Serial.println(F("[SPU] All Subsystems Initialized Successfully."));
    #endif
}

void SensorManager::loop() {
    unsigned long now = millis();

    // 1. High frequency sensor sampling
    if (now - _last_sample_time >= SENSOR_SAMPLE_INTERVAL) {
        _last_sample_time = now;

        gpsManager.update();
        envManager.update();
        mpuManager.update();
        gasManager.update();

        const EnvironmentData& env = envManager.getData();
        const MotionData& motion = mpuManager.getData();
        const GasData& gas = gasManager.getData();

        emergencyDetector.update(env, motion, gas);
    }

    // 2. Check if urgent emergency trigger occurred
    const EmergencyState& emergency = emergencyDetector.getState();
    unsigned long dispatch_interval = emergency.is_active ? EMERGENCY_SEND_INTERVAL : TELEMETRY_SEND_INTERVAL;

    if (now - _last_send_time >= dispatch_interval) {
        _last_send_time = now;

        const EnvironmentData& env = envManager.getData();
        const MotionData& motion = mpuManager.getData();
        const GasData& gas = gasManager.getData();
        const GPSData& gps = gpsManager.getData();

        SystemHealthMetrics health = HealthCalculator::calculate(env, motion, gas, gps);

        TelemetryPacket pkt = buildTelemetryPacket(env, motion, gas, gps, emergency, health);
        
        #if SPU_DEBUG_ENABLE
        printLiveSensorDiagnostics(env, motion, gas, gps, emergency, health);
        #endif

        // Blink LED on transmit
        digitalWrite(STATUS_LED_PIN, HIGH);
        uartManager.sendTelemetry(pkt);
        digitalWrite(STATUS_LED_PIN, LOW);
    }
}

TelemetryPacket SensorManager::buildTelemetryPacket(const EnvironmentData& env,
                                                   const MotionData& motion,
                                                   const GasData& gas,
                                                   const GPSData& gps,
                                                   const EmergencyState& emergency,
                                                   const SystemHealthMetrics& health) {
    TelemetryPacket pkt;
    memset(&pkt, 0, sizeof(TelemetryPacket));

    pkt.magic1 = PROTOCOL_MAGIC_BYTE1;
    pkt.magic2 = PROTOCOL_MAGIC_BYTE2;
    pkt.version = PROTOCOL_VERSION;
    pkt.node_id = SPU_DEVICE_ID;
    
    pkt.emergency_code = emergency.code;
    pkt.priority = emergency.priority;
    pkt.health_score = health.node_health_score;
    pkt.risk_score = health.environmental_risk_score;
    pkt.battery_percent = 100; // Default nominal 100% (Battery ADC disabled)

    // GPS conversion
    pkt.lat_deg_e7 = (int32_t)(gps.latitude * 1e7);
    pkt.lon_deg_e7 = (int32_t)(gps.longitude * 1e7);
    pkt.alt_meters = (int16_t)gps.altitude_m;
    pkt.sat_count = gps.satellites;
    pkt.gps_fix = gps.fix_valid ? 1 : 0;

    // Environment conversion
    pkt.temp_c_x10 = (int16_t)(env.temperature_c * 10.0f);
    pkt.humidity_x10 = (uint16_t)(env.humidity_pct * 10.0f);
    pkt.pressure_hpa = (uint16_t)env.pressure_hpa;
    pkt.gas_ppm = gas.ppm_estimate;

    // Motion conversion
    pkt.accel_x_mg = (int16_t)(motion.accel_x * 1000.0f);
    pkt.accel_y_mg = (int16_t)(motion.accel_y * 1000.0f);
    pkt.accel_z_mg = (int16_t)(motion.accel_z * 1000.0f);
    pkt.gyro_x_dps = (int16_t)motion.gyro_x;
    pkt.gyro_y_dps = (int16_t)motion.gyro_y;
    pkt.gyro_z_dps = (int16_t)motion.gyro_z;

    // Compute CRC-16 Checksum
    pkt.crc16 = calculate_crc16((const uint8_t*)&pkt, sizeof(TelemetryPacket) - sizeof(uint16_t));

    return pkt;
}

void SensorManager::printLiveSensorDiagnostics(const EnvironmentData& env,
                                                const MotionData& motion,
                                                const GasData& gas,
                                                const GPSData& gps,
                                                const EmergencyState& emergency,
                                                const SystemHealthMetrics& health) {
    Serial.println(F("\n┌─────────────────────────────────────────────────────────────┐"));
    Serial.println(F("│       SPU STEP-BY-STEP SENSOR DIAGNOSTIC REPORT             │"));
    Serial.println(F("└─────────────────────────────────────────────────────────────┘"));

    // STEP 1: Environmental Sensor
    Serial.println(F("\n[STEP 1/5] ENVIRONMENTAL SENSOR (DHT11 / DHT22)"));
    Serial.println(F("-------------------------------------------------------------"));
    if (env.valid) {
        Serial.printf("  • Temperature       : %.1f °C\n", env.temperature_c);
        Serial.printf("  • Humidity          : %.1f %%\n", env.humidity_pct);
        Serial.println(F("  • Sensor Status     : OK (Connected & Reading)"));
    } else {
        Serial.println(F("  • Sensor Status     : FAILED / NOT CONNECTED (Check Pin 23)"));
    }

    // STEP 2: Gas Sensor
    Serial.println(F("\n[STEP 2/5] AIR QUALITY & GAS SENSOR (MQ135)"));
    Serial.println(F("-------------------------------------------------------------"));
    Serial.printf("  • Gas Concentration : %u PPM\n", gas.ppm_estimate);
    Serial.printf("  • Pollution Warning : %s\n", gas.is_gas_warning ? "ALERT! HIGH POLLUTION" : "NO (Normal)");
    Serial.printf("  • Fire / Smoke Risk : %s\n", gas.is_gas_danger ? "CRITICAL! DANGER LEVEL" : "NO (Normal)");

    // STEP 3: Motion & IMU Sensor
    Serial.println(F("\n[STEP 3/5] MOTION & SEISMIC SENSOR (MPU6050 6-DOF IMU)"));
    Serial.println(F("-------------------------------------------------------------"));
    Serial.printf("  • Raw Accel (X,Y,Z) : X=%.2fG, Y=%.2fG, Z=%.2fG\n", motion.accel_x, motion.accel_y, motion.accel_z);
    Serial.printf("  • Gyro Rate (X,Y,Z) : X=%.1f°/s, Y=%.1f°/s, Z=%.1f°/s\n", motion.gyro_x, motion.gyro_y, motion.gyro_z);
    Serial.printf("  • Vector Magnitude  : %.2f G\n", motion.total_accel_g);
    Serial.printf("  • Dynamic Tilt Angle: %.1f°\n", motion.tilt_deg);
    Serial.printf("  • Motion Detected   : %s\n", motion.is_motion_detected ? "YES (Moving)" : "NO (Still)");
    Serial.printf("  • Sudden Impact     : %s\n", motion.is_sudden_impact ? "YES (Spike Detected!)" : "NO");
    Serial.printf("  • Free Fall         : %s\n", motion.is_free_fall ? "YES (Free Fall Detected!)" : "NO");
    Serial.printf("  • Seismic Anomaly   : %s\n", motion.is_earthquake_vibration ? "ALERT! SEISMIC VIBRATION" : "NO");

    // STEP 4: GPS Navigation Module
    Serial.println(F("\n[STEP 4/5] GPS NAVIGATION MODULE (NEO-6M)"));
    Serial.println(F("-------------------------------------------------------------"));
    if (gps.fix_valid) {
        Serial.println(F("  • GPS Fix Status    : VALID FIX (Lock Acquired)"));
        Serial.printf("  • Satellites in View: %d Satellites\n", gps.satellites);
        Serial.printf("  • Latitude          : %.6f°\n", gps.latitude);
        Serial.printf("  • Longitude         : %.6f°\n", gps.longitude);
        Serial.printf("  • Altitude          : %.1f meters\n", gps.altitude_m);
    } else {
        Serial.printf("  • GPS Fix Status    : SEARCHING FOR SATELLITES... (%d Seen)\n", gps.satellites);
        Serial.println(F("  • Tip               : Move antenna outdoors for satellite lock"));
    }

    // STEP 5: Emergency Engine & Health
    Serial.println(F("\n[STEP 5/5] EMERGENCY FUSION ENGINE & SYSTEM HEALTH"));
    Serial.println(F("-------------------------------------------------------------"));
    Serial.printf("  • Active Emergency  : '%c' (%s)\n", emergency.code, emergency.description);
    Serial.printf("  • Priority Level    : %d (1=Normal, 5=Critical)\n", emergency.priority);
    Serial.printf("  • Node Health Score : %d %%\n", health.node_health_score);
    Serial.printf("  • Env Risk Score    : %d %%\n", health.environmental_risk_score);
    Serial.println(F("=============================================================\n"));
}
