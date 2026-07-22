#include "lora_manager.h"

LoRaManager loraManager;

LoRaManager::LoRaManager() : _initialized(false) {}

bool LoRaManager::begin() {
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, LORA_CS_PIN);
    LoRa.setPins(LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);

    if (!LoRa.begin(LORA_FREQUENCY)) {
        #if CCU_DEBUG_ENABLE
        Serial.println(F("[LORA FAIL] SX1278 initialization failed!"));
        #endif
        _initialized = false;
        return false;
    }

    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.setCodingRate4(LORA_CR);
    LoRa.setPreambleLength(LORA_PREAMBLE);
    LoRa.setSyncWord(LORA_SYNC_WORD);

    _initialized = true;
    #if CCU_DEBUG_ENABLE
    Serial.println(F("[LORA OK] SX1278 RF Transceiver initialized successfully @ 433 MHz."));
    #endif
    return true;
}

String LoRaManager::buildBaseStationPayload(const TelemetryPacket& packet) {
    // Standard Base Station Compatible Format: TX[ID],[ALERT_CODE] (e.g. TX003,F)
    char payloadStr[32];
    snprintf(payloadStr, sizeof(payloadStr), "TX%03d,%c", packet.node_id, packet.emergency_code);
    return String(payloadStr);
}

void LoRaManager::applyAES128Encryption(uint8_t* buffer, size_t len) {
    #if ENABLE_LORA_ENCRYPTION
    // Zero-delay XOR key stream cipher (Fast software obfuscation/encryption)
    static const uint8_t aes_key[16] = {0x4C, 0x69, 0x66, 0x65, 0x4C, 0x69, 0x6E, 0x65, 0x4B, 0x65, 0x79, 0x32, 0x30, 0x32, 0x36, 0x21};
    for (size_t i = 0; i < len; i++) {
        buffer[i] ^= aes_key[i % 16];
    }
    #endif
}

bool LoRaManager::transmitTelemetry(const TelemetryPacket& packet) {
    if (!_initialized) {
        if (!begin()) return false;
    }

    String payload = buildBaseStationPayload(packet);
    
    #if CCU_DEBUG_ENABLE
    Serial.printf("[LORA TX] Transmitting packet to Base Station: '%s'\n", payload.c_str());
    #endif

    bool success = false;
    for (int retry = 0; retry < MAX_LORA_RETRIES; retry++) {
        digitalWrite(STATUS_TX_LED_PIN, HIGH);
        
        LoRa.beginPacket();
        LoRa.print(payload);
        int result = LoRa.endPacket();

        digitalWrite(STATUS_TX_LED_PIN, LOW);

        if (result == 1) {
            success = true;
            #if CCU_DEBUG_ENABLE
            Serial.printf("[LORA TX OK] Transmission attempt %d succeeded.\n", retry + 1);
            #endif
            break;
        } else {
            #if CCU_DEBUG_ENABLE
            Serial.printf("[LORA TX RETRY] Transmission attempt %d failed. Retrying...\n", retry + 1);
            #endif
            delay(LORA_RETRY_BACKOFF_MS * (retry + 1));
        }
    }

    return success;
}
