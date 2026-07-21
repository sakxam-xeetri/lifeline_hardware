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
    uint8_t dataOnState = LED_DATA_ACTIVE_HIGH ? HIGH : LOW;
    digitalWrite(LED_DATA, dataOnState);
    rxBlinkEndTime = millis() + RX_BLINK_DURATION_MS;
}

void updateLEDs() {
    unsigned long now = millis();

    // Strictly sync wifiConnected variable with actual hardware WiFi status
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    wifiConnected = isConnected;

    uint8_t wifiOnState  = LED_WIFI_ACTIVE_HIGH ? HIGH : LOW;
    uint8_t wifiOffState = LED_WIFI_ACTIVE_HIGH ? LOW  : HIGH;
    uint8_t dataOnState  = LED_DATA_ACTIVE_HIGH ? HIGH : LOW;
    uint8_t dataOffState = LED_DATA_ACTIVE_HIGH ? LOW  : HIGH;

    // 1. WiFi LED status update
    if (portalActive || currentScreen == SCREEN_PORTAL) {
        // Fast blink (100 ms interval)
        if (now - lastWiFiBlinkTime >= 100) {
            lastWiFiBlinkTime = now;
            wifiBlinkState = !wifiBlinkState;
            digitalWrite(LED_WIFI, wifiBlinkState ? wifiOnState : wifiOffState);
        }
    } else if (currentScreen == SCREEN_COUNTDOWN) {
        // Slow blink (500 ms interval)
        if (now - lastWiFiBlinkTime >= 500) {
            lastWiFiBlinkTime = now;
            wifiBlinkState = !wifiBlinkState;
            digitalWrite(LED_WIFI, wifiBlinkState ? wifiOnState : wifiOffState);
        }
    } else if (isConnected) {
        digitalWrite(LED_WIFI, wifiOnState);
    } else {
        digitalWrite(LED_WIFI, wifiOffState);
    }

    // 2. Data RX LED blink timeout check
    if (rxBlinkEndTime > 0 && millis() < rxBlinkEndTime) {
        digitalWrite(LED_DATA, dataOnState);
    } else {
        digitalWrite(LED_DATA, dataOffState);
        rxBlinkEndTime = 0;
    }
}
