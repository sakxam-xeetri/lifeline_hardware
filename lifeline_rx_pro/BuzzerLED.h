#ifndef BUZZER_LED_H
#define BUZZER_LED_H

#include "Config.h"

// Initialize Pins for buzzer and LEDs
void initBuzzerLED();

// Alarm feedback tones
void playBootTone();
void playAlertTone(int priority);

// Status/Data LED indicators
void triggerRxBlink();
void updateLEDs();

#endif // BUZZER_LED_H
