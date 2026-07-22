#ifndef GAS_MANAGER_H
#define GAS_MANAGER_H

#include <Arduino.h>
#include "ConfigSPU.h"

struct GasData {
    uint16_t ppm_estimate;
    bool is_gas_warning;
    bool is_gas_danger;
    bool valid;
};

class GasManager {
public:
    GasManager();
    void begin();
    void update();
    const GasData& getData() const { return _data; }

private:
    GasData _data;
    uint16_t readPPM();
};

extern GasManager gasManager;

#endif // GAS_MANAGER_H
