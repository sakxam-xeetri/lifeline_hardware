#include "environment_manager.h"

EnvironmentManager envManager;

EnvironmentManager::EnvironmentManager() : _dht(DHT_PIN, DHT_TYPE) {
    _data = {25.0f, 50.0f, 1013.0f, false};
}

void EnvironmentManager::begin() {
    _dht.begin();
    #if SPU_DEBUG_ENABLE
    Serial.println(F("[ENV] DHT Sensor Initialized."));
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
}
