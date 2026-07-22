#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <Arduino.h>

struct DiagnosticsData {
    uint32_t total_packets_received;
    uint32_t total_packets_transmitted;
    uint32_t crc_errors;
    uint32_t lora_tx_failures;
};

class Diagnostics {
public:
    Diagnostics();
    void recordPacketReceived();
    void recordCRCError();
    void recordPacketTransmitted();
    void recordTxFailure();
    void printReport();

private:
    DiagnosticsData _data;
};

extern Diagnostics diagnostics;

#endif // DIAGNOSTICS_H
