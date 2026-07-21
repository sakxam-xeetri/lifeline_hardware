#ifndef BUZZER_LED_H
#define BUZZER_LED_H

#include "Config.h"

// Initialize Pins for buzzer and LEDs
void initBuzzerLED();

// Alarm & UI feedback tones
void playBootTone();
void playWiFiSuccessTone();
void playWiFiFailTone();
void playCountdownTickTone();
void playPortalOpenTone();
void playSkipConfirmTone();
void playReturnIdleTone();
void playAlertTone(int priority);

// Status/Data LED indicators
void triggerRxBlink();
void updateLEDs();

#endif // BUZZER_LED_H
