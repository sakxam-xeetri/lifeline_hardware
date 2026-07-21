#ifndef LORA_COMM_H
#define LORA_COMM_H

#include "Config.h"

extern bool loraInitialized;

bool initLoRa();
bool parseLoRaPacket(int& deviceId, int& alertIndex, int& rssi);

#endif // LORA_COMM_H
