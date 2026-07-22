#include "power_manager.h"
#include "esp_sleep.h"

void PowerManager::init() {
    pinMode(STATUS_TX_LED_PIN, OUTPUT);
    digitalWrite(STATUS_TX_LED_PIN, LOW);
    #if CCU_DEBUG_ENABLE
    Serial.println(F("[POWER] Power & Sleep Manager Initialized."));
    #endif
}

void PowerManager::enterLightSleep(uint32_t sleep_ms) {
    if (sleep_ms == 0) return;
    esp_sleep_enable_timer_wakeup(sleep_ms * 1000ULL);
    esp_light_sleep_start();
}
