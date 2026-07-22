#ifndef UART_RECEIVER_H
#define UART_RECEIVER_H

#include <Arduino.h>
#include "ConfigCCU.h"
#include "SharedProtocol.h"

class UARTReceiver {
public:
    UARTReceiver();
    void begin();
    bool readTelemetryPacket(TelemetryPacket& outPacket);

private:
    HardwareSerial _spuSerial;
    uint8_t _rx_buffer[sizeof(TelemetryPacket) * 2];
    size_t _rx_idx;
};

extern UARTReceiver uartReceiver;

#endif // UART_RECEIVER_H
