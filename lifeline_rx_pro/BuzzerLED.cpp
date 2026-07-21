#include "BuzzerLED.h"
#include "DisplayUI.h"
#include <WiFi.h>

extern bool wifiConnected;
extern bool portalActive;
extern ScreenState currentScreen;

static unsigned long rxBlinkEndTime = 0;
static unsigned long lastWiFiBlinkTime = 0;
static bool wifiBlinkState = false;

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

void playWiFiSuccessTone() {
    tone(BUZZER_PIN, 1000, 80);
    delay(100);
    tone(BUZZER_PIN, 1500, 80);
}

void playWiFiFailTone() {
    tone(BUZZER_PIN, 1500, 80);
    delay(100);
    tone(BUZZER_PIN, 1000, 80);
}

void playCountdownTickTone() {
    tone(BUZZER_PIN, 2000, 50);
}

void playPortalOpenTone() {
    tone(BUZZER_PIN, 1800, 150);
}

void playSkipConfirmTone() {
    tone(BUZZER_PIN, 1500, 50);
}

void playReturnIdleTone() {
    tone(BUZZER_PIN, 1200, 80);
}

void playAlertTone(int priority) {
    switch (priority) {
        case 0:  // Critical - 3x rapid beeps
            tone(BUZZER_PIN, 2500, 100);
            delay(220);
            tone(BUZZER_PIN, 2500, 100);
            delay(220);
            tone(BUZZER_PIN, 2500, 100);
            break;
        case 1:  // High - 2x beeps
            tone(BUZZER_PIN, 2200, 150);
            delay(330);
            tone(BUZZER_PIN, 2200, 150);
            break;
        default: // Medium / OK / Info - 1x single beep
            tone(BUZZER_PIN, 2000, 200);
            break;
    }
}

void triggerRxBlink() {
    digitalWrite(LED_DATA, HIGH);
    rxBlinkEndTime = millis() + RX_BLINK_DURATION_MS;
}

void updateLEDs() {
    unsigned long now = millis();

    // 1. WiFi LED status update
    if (portalActive || currentScreen == SCREEN_PORTAL) {
        // Fast blink (100 ms interval)
        if (now - lastWiFiBlinkTime >= 100) {
            lastWiFiBlinkTime = now;
            wifiBlinkState = !wifiBlinkState;
            digitalWrite(LED_WIFI, wifiBlinkState ? HIGH : LOW);
        }
    } else if (currentScreen == SCREEN_COUNTDOWN) {
        // Slow blink (500 ms interval)
        if (now - lastWiFiBlinkTime >= 500) {
            lastWiFiBlinkTime = now;
            wifiBlinkState = !wifiBlinkState;
            digitalWrite(LED_WIFI, wifiBlinkState ? HIGH : LOW);
        }
    } else if (WiFi.status() == WL_CONNECTED || wifiConnected) {
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
