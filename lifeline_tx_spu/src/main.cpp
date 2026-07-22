#include <Arduino.h>
#include "ConfigSPU.h"
#include "sensor_manager.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    #if SPU_DEBUG_ENABLE
    Serial.println(F("\n=================================================="));
    Serial.println(F("    LIFELINE SENSOR PROCESSING UNIT (SPU)         "));
    Serial.printf( "    Device ID: %d | Firmware: %s                   \n", SPU_DEVICE_ID, SPU_FIRMWARE_VERSION);
    Serial.println(F("==================================================\n"));
    #endif

    sensorManager.begin();
}

void loop() {
    sensorManager.loop();
}
