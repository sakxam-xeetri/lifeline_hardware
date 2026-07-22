#ifndef SHARED_PROTOCOL_H
#define SHARED_PROTOCOL_H

#include <stdint.h>

// Packet Constants
#define PROTOCOL_MAGIC_BYTE1  'L'
#define PROTOCOL_MAGIC_BYTE2  'F'
#define PROTOCOL_VERSION     0x01

// Emergency Codes
#define EMERGENCY_NONE        'N' // Normal telemetry
#define EMERGENCY_GENERAL     'E' // Emergency
#define EMERGENCY_MEDICAL     'M' // Medical Emergency
#define EMERGENCY_FIRE        'F' // Fire / High Gas + Temp
#define EMERGENCY_EARTHQUAKE  'Q' // Continuous high vibration
#define EMERGENCY_LANDSLIDE   'L' // Sudden motion + tilt / pressure change
#define EMERGENCY_HEAT        'H' // Extreme Heat wave
#define EMERGENCY_COLD        'C' // Extreme Cold wave
#define EMERGENCY_HUMIDITY    'W' // Extreme Humidity
#define EMERGENCY_GAS         'G' // High Gas leak / Smoke
#define EMERGENCY_BATTERY     'B' // Low Battery
#define EMERGENCY_SOS         'S' // Manual SOS button pressed

// Emergency Priority Levels
#define PRIORITY_NORMAL       1
#define PRIORITY_LOW          2
#define PRIORITY_MEDIUM       3
#define PRIORITY_HIGH         4
#define PRIORITY_CRITICAL     5

#pragma pack(push, 1)
typedef struct {
    uint8_t  magic1;             // 'L'
    uint8_t  magic2;             // 'F'
    uint8_t  version;            // 0x01
    uint16_t node_id;            // Transmitter Node ID
    uint8_t  emergency_code;     // Emergency ASCII character code
    uint8_t  priority;           // 1 to 5 priority level
    uint8_t  health_score;       // 0 to 100%
    uint8_t  risk_score;         // 0 to 100%
    uint8_t  battery_percent;    // 0 to 100%
    
    // GPS Data
    int32_t  lat_deg_e7;         // Lat * 10,000,000
    int32_t  lon_deg_e7;         // Lon * 10,000,000
    int16_t  alt_meters;         // Altitude in meters
    uint8_t  sat_count;          // Number of satellites
    uint8_t  gps_fix;            // 1 if fix valid, 0 if invalid
    
    // Environmental Data
    int16_t  temp_c_x10;        // Temperature °C * 10
    uint16_t humidity_x10;       // Humidity % * 10
    uint16_t pressure_hpa;       // Atmospheric pressure in hPa
    uint16_t gas_ppm;            // Air Quality MQ135 PPM estimate
    
    // IMU MPU6050 Motion Data
    int16_t  accel_x_mg;         // Accel X in mG
    int16_t  accel_y_mg;         // Accel Y in mG
    int16_t  accel_z_mg;         // Accel Z in mG
    int16_t  gyro_x_dps;         // Gyro X in deg/sec
    int16_t  gyro_y_dps;         // Gyro Y in deg/sec
    int16_t  gyro_z_dps;         // Gyro Z in deg/sec
    
    uint16_t crc16;              // CRC-16-CCITT Checksum over preceding bytes
} TelemetryPacket;
#pragma pack(pop)

// Helper function to calculate CRC-16-CCITT
inline uint16_t calculate_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

#endif // SHARED_PROTOCOL_H
