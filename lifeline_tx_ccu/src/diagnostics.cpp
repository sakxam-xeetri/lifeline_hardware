#include "diagnostics.h"

Diagnostics diagnostics;

Diagnostics::Diagnostics() {
    _data = {0, 0, 0, 0};
}

void Diagnostics::recordPacketReceived() {
    _data.total_packets_received++;
}

void Diagnostics::recordCRCError() {
    _data.crc_errors++;
}

void Diagnostics::recordPacketTransmitted() {
    _data.total_packets_transmitted++;
}

void Diagnostics::recordTxFailure() {
    _data.lora_tx_failures++;
}

void Diagnostics::printReport() {
    Serial.println(F("\n--- CCU TELEMETRY DIAGNOSTICS ---"));
    Serial.printf(" Packets Received from SPU : %u\n", _data.total_packets_received);
    Serial.printf(" Packets Transmitted (LoRa): %u\n", _data.total_packets_transmitted);
    Serial.printf(" CRC Checksum Errors      : %u\n", _data.crc_errors);
    Serial.printf(" LoRa TX Failures         : %u\n", _data.lora_tx_failures);
    Serial.printf(" Free Heap Memory         : %u bytes\n", ESP.getFreeHeap());
    Serial.println(F("----------------------------------\n"));
}
