#ifndef ENVIRONMENT_MANAGER_H
#define ENVIRONMENT_MANAGER_H

#include <Arduino.h>
#include <DHT.h>
#include "ConfigSPU.h"

struct EnvironmentData {
    float temperature_c;
    float humidity_pct;
    float pressure_hpa; // 1013.25 default unless BME280 connected
    bool valid;
};

class EnvironmentManager {
public:
    EnvironmentManager();
    void begin();
    void update();
    const EnvironmentData& getData() const { return _data; }

private:
    DHT _dht;
    EnvironmentData _data;
};

extern EnvironmentManager envManager;

#endif // ENVIRONMENT_MANAGER_H
