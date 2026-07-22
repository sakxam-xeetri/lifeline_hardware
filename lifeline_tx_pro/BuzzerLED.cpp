#include "BuzzerLED.h"

void initBuzzerLED() {
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    clearAllLEDs();
}

void setLED(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

void clearAllLEDs() {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
}

void playSuccessTone() {
    tone(BUZZER_PIN, 2200, 80);  // Short, non-blocking
}

void playErrorTone() {
    tone(BUZZER_PIN, 800, 150);  // Single beep
}

void playConfirmTone() {
    tone(BUZZER_PIN, 2500, 30);  // Very short
}

void playClickTone() {
    tone(BUZZER_PIN, 1800, 15);  // Minimal click
}

void playNavigateTone() {
    tone(BUZZER_PIN, 1500, 10);  // Ultra-fast navigation beep
}
