#include "uart_receiver.h"

UARTReceiver uartReceiver;

UARTReceiver::UARTReceiver() : _spuSerial(2), _rx_idx(0) {}

void UARTReceiver::begin() {
    _spuSerial.begin(UART_SPU_BAUD, SERIAL_8N1, UART_SPU_RX_PIN, UART_SPU_TX_PIN);
    #if CCU_DEBUG_ENABLE
    Serial.println(F("[CCU UART] Initialized Hardware Serial 2 for SPU link."));
    #endif
}

bool UARTReceiver::readTelemetryPacket(TelemetryPacket& outPacket) {
    while (_spuSerial.available() > 0) {
        uint8_t byteIn = _spuSerial.read();

        // Looking for start header magic bytes
        if (_rx_idx == 0 && byteIn != PROTOCOL_MAGIC_BYTE1) {
            continue;
        }
        if (_rx_idx == 1 && byteIn != PROTOCOL_MAGIC_BYTE2) {
            _rx_idx = 0;
            if (byteIn == PROTOCOL_MAGIC_BYTE1) {
                _rx_buffer[0] = byteIn;
                _rx_idx = 1;
            }
            continue;
        }

        _rx_buffer[_rx_idx++] = byteIn;

        // Received full structure length
        if (_rx_idx >= sizeof(TelemetryPacket)) {
            _rx_idx = 0; // Reset index for next packet

            TelemetryPacket* candidate = (TelemetryPacket*)_rx_buffer;
            
            // Validate CRC16 checksum
            uint16_t expected_crc = calculate_crc16((const uint8_t*)candidate, sizeof(TelemetryPacket) - sizeof(uint16_t));
            if (candidate->crc16 == expected_crc) {
                memcpy(&outPacket, candidate, sizeof(TelemetryPacket));
                #if CCU_DEBUG_ENABLE
                Serial.printf("[CCU UART RX] Valid Packet! Node: %d, Code: '%c', Risk: %d%%\n",
                              outPacket.node_id, outPacket.emergency_code, outPacket.risk_score);
                #endif
                return true;
            } else {
                #if CCU_DEBUG_ENABLE
                Serial.printf("[CCU UART RX] CRC Mismatch! Expected 0x%04X, Got 0x%04X\n", expected_crc, candidate->crc16);
                #endif
            }
        }
    }
    return false;
}
