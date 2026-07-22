#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include "ConfigSPU.h"

struct GPSData {
    double latitude;
    double longitude;
    float altitude_m;
    float speed_kmh;
    uint8_t satellites;
    bool fix_valid;
    uint32_t utc_time;
};

class GPSManager {
public:
    GPSManager();
    void begin();
    void update();
    const GPSData& getData() const { return _data; }
    
private:
    TinyGPSPlus _gps;
    HardwareSerial _gpsSerial;
    GPSData _data;
};

extern GPSManager gpsManager;

#endif // GPS_MANAGER_H
