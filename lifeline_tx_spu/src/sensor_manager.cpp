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
    pkt.battery_percent = env.battery_percent;

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
    Serial.println(F("\n=================================================="));
    Serial.println(F("           SPU LIVE SENSOR DIAGNOSTIC DASHBOARD   "));
    Serial.println(F("=================================================="));

    // 1. Environmental Sensor (DHT11/22 & Battery)
    if (env.valid) {
        Serial.printf("[ENV] Temp: %.1f °C | Humidity: %.1f %% | Batt: %.2fV (%d%%)\n",
                      env.temperature_c, env.humidity_pct, env.battery_voltage, env.battery_percent);
    } else {
        Serial.println(F("[ENV] DHT Sensor Read Failure / Not Detected!"));
    }

    // 2. Gas Sensor (MQ135)
    Serial.printf("[GAS] MQ135 PPM: %u PPM | Warning: %s | Danger: %s\n",
                  gas.ppm_estimate,
                  gas.is_gas_warning ? "YES" : "NO",
                  gas.is_gas_danger ? "YES" : "NO");

    // 3. Motion & IMU Sensor (MPU6050)
    Serial.printf("[MPU] Accel (G): X=%.2f Y=%.2f Z=%.2f | Total: %.2fG | Tilt: %.1f°\n",
                  motion.accel_x, motion.accel_y, motion.accel_z, motion.total_accel_g, motion.tilt_deg);
    Serial.printf("[MPU] Flags: Motion=%s | Impact=%s | Fall=%s | Earthquake=%s\n",
                  motion.is_motion_detected ? "YES" : "NO",
                  motion.is_sudden_impact ? "YES" : "NO",
                  motion.is_free_fall ? "YES" : "NO",
                  motion.is_earthquake_vibration ? "YES" : "NO");

    // 4. GPS Module (NEO-6M)
    if (gps.fix_valid) {
        Serial.printf("[GPS] Fix: VALID (%d Sats) | Lat: %.6f | Lon: %.6f | Alt: %.1fm\n",
                      gps.satellites, gps.latitude, gps.longitude, gps.altitude_m);
    } else {
        Serial.printf("[GPS] Fix: SEARCHING... (%d Satellites Seen)\n", gps.satellites);
    }

    // 5. Emergency Engine & Health Scores
    Serial.printf("[SYS] Emergency Status: '%c' (%s) | Priority: %d\n",
                  emergency.code, emergency.description, emergency.priority);
    Serial.printf("[SYS] Health Score: %d%% | Environmental Risk Score: %d%%\n",
                  health.node_health_score, health.environmental_risk_score);
    Serial.println(F("=================================================="));
}
