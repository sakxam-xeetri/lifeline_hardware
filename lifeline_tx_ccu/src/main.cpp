#include <Arduino.h>
#include "ConfigCCU.h"
#include "SharedProtocol.h"
#include "uart_receiver.h"
#include "lora_manager.h"
#include "power_manager.h"
#include "diagnostics.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    #if CCU_DEBUG_ENABLE
    Serial.println(F("\n=================================================="));
    Serial.println(F("  LIFELINE COMMUNICATION CONTROLLER UNIT (CCU)    "));
    Serial.printf( "  Device ID: %d | Firmware: %s                   \n", CCU_DEVICE_ID, CCU_FIRMWARE_VERSION);
    Serial.println(F("==================================================\n"));
    #endif

    PowerManager::init();
    uartReceiver.begin();
    loraManager.begin();
}

void loop() {
    TelemetryPacket pkt;
    
    // Check if a valid UART telemetry packet has been received from SPU
    if (uartReceiver.readTelemetryPacket(pkt)) {
        diagnostics.recordPacketReceived();

        // Transmit packet over LoRa SX1278
        bool success = loraManager.transmitTelemetry(pkt);
        if (success) {
            diagnostics.recordPacketTransmitted();
        } else {
            diagnostics.recordTxFailure();
        }
    }

    // Yield CPU time to FreeRTOS idle task
    vTaskDelay(10 / portTICK_PERIOD_MS);
}
