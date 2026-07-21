#include "BuzzerLED.h"
#include <WiFi.h>

extern bool wifiConnected;

static unsigned long rxBlinkEndTime = 0;

void initBuzzerLED() {
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_WIFI, OUTPUT);
    pinMode(LED_DATA, OUTPUT);
    
    digitalWrite(LED_WIFI, LOW);
    digitalWrite(LED_DATA, LOW);
}

void playBootTone() {
    tone(BUZZER_PIN, 1500, 80);
}

void playAlertTone(int priority) {
    switch (priority) {
        case 0:  // Critical - triple beep
            tone(BUZZER_PIN, 2500, 100);
            delay(120);
            tone(BUZZER_PIN, 2500, 100);
            delay(120);
            tone(BUZZER_PIN, 2500, 100);
            break;
        case 1:  // High - double beep
            tone(BUZZER_PIN, 2200, 150);
            delay(180);
            tone(BUZZER_PIN, 2200, 150);
            break;
        default: // Others - single beep
            tone(BUZZER_PIN, 2000, 200);
            break;
    }
}

void triggerRxBlink() {
    digitalWrite(LED_DATA, HIGH);
    rxBlinkEndTime = millis() + RX_BLINK_DURATION_MS;
}

void updateLEDs() {
    // 1. WiFi LED status update (HIGH when connected, LOW otherwise)
    if (WiFi.status() == WL_CONNECTED || wifiConnected) {
        digitalWrite(LED_WIFI, HIGH);
    } else {
        digitalWrite(LED_WIFI, LOW);
    }

    // 2. Data RX LED blink timeout check
    if (rxBlinkEndTime > 0 && millis() >= rxBlinkEndTime) {
        digitalWrite(LED_DATA, LOW);
        rxBlinkEndTime = 0;
    }
}
