#ifndef LORA_COMM_H
#define LORA_COMM_H

#include "Config.h"
#include <SPI.h>
#include <LoRa.h>

void initLoRa();
bool transmitAlert();

#endif // LORA_COMM_H
