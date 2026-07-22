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
