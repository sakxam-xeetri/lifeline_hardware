#include "environment_manager.h"

EnvironmentManager envManager;

EnvironmentManager::EnvironmentManager() : _dht(DHT_PIN, DHT_TYPE) {
    _data = {25.0f, 50.0f, 1013.0f, 4.0f, 100, false};
}

void EnvironmentManager::begin() {
    _dht.begin();
    pinMode(BATTERY_ADC_PIN, INPUT);
    #if SPU_DEBUG_ENABLE
    Serial.println(F("[ENV] DHT Sensor and Battery ADC Initialized."));
    #endif
}

void EnvironmentManager::update() {
    float t = _dht.readTemperature();
    float h = _dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
        _data.temperature_c = t;
        _data.humidity_pct = h;
        _data.valid = true;
    } else {
        _data.valid = false;
    }
    
    // Standard nominal atmospheric pressure estimation (unless upgraded to BME280)
    _data.pressure_hpa = 1013.25f;

    readBattery();
}

void EnvironmentManager::readBattery() {
    int raw = analogRead(BATTERY_ADC_PIN);
    // ADC 12-bit (0-4095), 3.3V reference, 1:2 resistor divider ratio
    float v = ((float)raw / 4095.0f) * 3.3f * 2.0f;
    _data.battery_voltage = v;

    if (v >= BATTERY_MAX_V) {
        _data.battery_percent = 100;
    } else if (v <= BATTERY_MIN_V) {
        _data.battery_percent = 0;
    } else {
        _data.battery_percent = (uint8_t)(((v - BATTERY_MIN_V) / (BATTERY_MAX_V - BATTERY_MIN_V)) * 100.0f);
    }
}
