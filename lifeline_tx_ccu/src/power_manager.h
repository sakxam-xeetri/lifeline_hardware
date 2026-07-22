#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "ConfigCCU.h"

class PowerManager {
public:
    static void init();
    static void enterLightSleep(uint32_t sleep_ms);
};

#endif // POWER_MANAGER_H
