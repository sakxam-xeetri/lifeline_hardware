#include "uart_manager.h"

UARTManager uartManager;

UARTManager::UARTManager() : _ccuSerial(2) {}

void UARTManager::begin() {
    _ccuSerial.begin(UART_CCU_BAUD, SERIAL_8N1, UART_CCU_RX_PIN, UART_CCU_TX_PIN);
    #if SPU_DEBUG_ENABLE
    Serial.println(F("[UART] Hardware Serial 2 initialized for CCU link @ 115200 baud."));
    #endif
}

void UARTManager::sendTelemetry(const TelemetryPacket& packet) {
    // Write packet bytes directly to UART2
    size_t written = _ccuSerial.write((const uint8_t*)&packet, sizeof(TelemetryPacket));
    _ccuSerial.flush();

    #if SPU_DEBUG_ENABLE
    Serial.printf("[UART TX] Sent Telemetry Packet (%d bytes). Code: '%c', Priority: %d, Risk: %d%%\n",
                  written, packet.emergency_code, packet.priority, packet.risk_score);
    #endif
}
