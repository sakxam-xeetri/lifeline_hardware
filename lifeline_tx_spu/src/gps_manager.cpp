#include "gps_manager.h"

GPSManager gpsManager;

GPSManager::GPSManager() : _gpsSerial(1) {
    _data = {0.0, 0.0, 0.0f, 0.0f, 0, false, 0};
}

void GPSManager::begin() {
    _gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    #if SPU_DEBUG_ENABLE
    Serial.println(F("[GPS] Hardware Serial 1 initialized for NEO-6M GPS."));
    #endif
}

void GPSManager::update() {
    while (_gpsSerial.available() > 0) {
        _gps.encode(_gpsSerial.read());
    }

    if (_gps.location.isValid()) {
        _data.latitude = _gps.location.lat();
        _data.longitude = _gps.location.lng();
        _data.fix_valid = true;
    } else {
        _data.fix_valid = false;
    }

    if (_gps.altitude.isValid()) {
        _data.altitude_m = _gps.altitude.meters();
    }

    if (_gps.speed.isValid()) {
        _data.speed_kmh = _gps.speed.kmph();
    }

    if (_gps.satellites.isValid()) {
        _data.satellites = _gps.satellites.value();
    } else {
        _data.satellites = 0;
    }

    if (_gps.time.isValid()) {
        _data.utc_time = _gps.time.value();
    }
}
