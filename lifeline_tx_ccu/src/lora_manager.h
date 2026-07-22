#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "ConfigCCU.h"
#include "SharedProtocol.h"

class LoRaManager {
public:
    LoRaManager();
    bool begin();
    bool transmitTelemetry(const TelemetryPacket& packet);

private:
    bool _initialized;
    String buildBaseStationPayload(const TelemetryPacket& packet);
    void applyAES128Encryption(uint8_t* buffer, size_t len);
};

extern LoRaManager loraManager;

#endif // LORA_MANAGER_H
