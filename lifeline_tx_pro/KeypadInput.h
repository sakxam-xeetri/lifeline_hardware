#ifndef KEYPAD_INPUT_H
#define KEYPAD_INPUT_H

#include "Config.h"
#include "BuzzerLED.h"
#include "DisplayUI.h"
#include "LoRaComm.h"
#include <Keypad.h>

extern Keypad keypad;

void initKeypad();
char readSerialKey();
void printDebugHeader();

void handleKeyPress(char key);
void handleMenuInput(char key);
void handleConfirmInput(char key);
void handleResultInput(char key);
void handleSystemInfoInput(char key);
void handleUserManualInput(char key);

#endif // KEYPAD_INPUT_H
