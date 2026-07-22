#ifndef UART_MANAGER_H
#define UART_MANAGER_H

#include <Arduino.h>
#include "ConfigSPU.h"
#include "SharedProtocol.h"

class UARTManager {
public:
    UARTManager();
    void begin();
    void sendTelemetry(const TelemetryPacket& packet);

private:
    HardwareSerial _ccuSerial;
};

extern UARTManager uartManager;

#endif // UART_MANAGER_H
