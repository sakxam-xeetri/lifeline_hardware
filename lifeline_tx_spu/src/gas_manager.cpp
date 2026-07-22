#include "gas_manager.h"

GasManager gasManager;

GasManager::GasManager() {
    _data = {100, false, false, true};
}

void GasManager::begin() {
    pinMode(MQ135_PIN, INPUT);
    #if SPU_DEBUG_ENABLE
    Serial.println(F("[GAS] MQ135 Pin Initialized."));
    #endif
}

void GasManager::update() {
    uint16_t ppm = readPPM();
    _data.ppm_estimate = ppm;
    _data.is_gas_warning = (ppm >= THRESHOLD_GAS_WARNING);
    _data.is_gas_danger = (ppm >= THRESHOLD_GAS_DANGER);
    _data.valid = true;
}

uint16_t GasManager::readPPM() {
    int raw = analogRead(MQ135_PIN);
    // Rough linear mapping of 12-bit ADC (0-4095) to estimated PPM range (50 - 1000 PPM)
    float ppm = 50.0f + ((float)raw / 4095.0f) * 950.0f;
    return (uint16_t)ppm;
}
