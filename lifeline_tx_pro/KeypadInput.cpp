#include "KeypadInput.h"

const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 4;

// Standard 4x4 matrix keypad layout
char keypadLayout[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},   // Row 0: Numbers 1-3, Scroll Up
    {'4', '5', '6', 'B'},   // Row 1: Numbers 4-6, Scroll Down
    {'7', '8', '9', 'C'},   // Row 2: Numbers 7-9, System Info
    {'*', '0', '#', 'D'}    // Row 3: Confirm, 0, Cancel, Help
};

// Keypad GPIO pins - WORKING CONFIGURATION
byte rowPins[KEYPAD_ROWS] = {32, 33, 25, 26};  // Connect to keypad rows
byte colPins[KEYPAD_COLS] = {14, 12, 13, 15};  // Connect to keypad columns

// Keypad instance definition
Keypad keypad = Keypad(makeKeymap(keypadLayout), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

void initKeypad() {
    // Keypad constructor automatically handles pin setup
}

char readSerialKey() {
    if (!Serial.available()) return '\0';
    
    char c = Serial.read();
    while (Serial.available()) Serial.read();
    
    if (c >= 'a' && c <= 'd') c = c - 32;
    if (c == 's' || c == 'S') c = '*';
    if (c == 'x' || c == 'X') c = '#';
    
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'D') || c == '*' || c == '#') {
        return c;
    }
    
    if (c != '\n' && c != '\r') {
        Serial.println(F("\n[DEBUG] Keys: 0-9=Select, A=Up, B=Down, C=Info, D=Help, S/*=OK, X/#=Cancel"));
    }
    return '\0';
}

void printDebugHeader() {
    Serial.println(F("\n╔═══════════════════════════════════════════════════════════╗"));
    Serial.println(F("║       LIFELINE EMERGENCY TRANSMITTER v3.1 PRO             ║"));
    Serial.println(F("║              Serial Debug Mode Active                     ║"));
    Serial.println(F("╚═══════════════════════════════════════════════════════════╝\n"));
}

void handleKeyPress(char key) {
    if (key == '\0') return;
    
    Serial.printf("[INPUT] Key: %c, Screen: %d\n", key, currentScreen);
    
    switch (currentScreen) {
        case SCREEN_MENU:
            handleMenuInput(key);
            break;
        case SCREEN_CONFIRM:
            handleConfirmInput(key);
            break;
        case SCREEN_RESULT:
            handleResultInput(key);
            break;
        case SCREEN_SYSTEM_INFO:
            handleSystemInfoInput(key);
            break;
        case SCREEN_USER_MANUAL:
            handleUserManualInput(key);
            break;
        default:
            break;
    }
    
    lastKeyPressTime = millis();
}

void handleMenuInput(char key) {
    int oldSelection = selectedAlertIndex;
    int oldScrollOffset = menuScrollOffset;
    bool selectionChanged = false;
    
    if (key >= '1' && key <= '9') {
        int targetIndex = key - '1';
        if (targetIndex < ALERT_COUNT) {
            selectedAlertIndex = targetIndex;
            previousScreen = SCREEN_MENU;
            currentScreen = SCREEN_CONFIRM;
            drawConfirmScreen();
            return;
        }
    }
    else if (key == '0' && 9 < ALERT_COUNT) {
        selectedAlertIndex = 9;
        previousScreen = SCREEN_MENU;
        currentScreen = SCREEN_CONFIRM;
        drawConfirmScreen();
        return;
    }
    else if (key == 'A') {
        selectedAlertIndex = (selectedAlertIndex > 0) ? selectedAlertIndex - 1 : ALERT_COUNT - 1;
        updateMenuScroll();
        selectionChanged = true;
        playNavigateTone();
    }
    else if (key == 'B') {
        selectedAlertIndex = (selectedAlertIndex < ALERT_COUNT - 1) ? selectedAlertIndex + 1 : 0;
        updateMenuScroll();
        selectionChanged = true;
        playNavigateTone();
    }
    else if (key == '*') {
        previousScreen = SCREEN_MENU;
        currentScreen = SCREEN_CONFIRM;
        drawConfirmScreen();
        return;
    }
    else if (key == 'C') {
        previousScreen = SCREEN_MENU;
        currentScreen = SCREEN_SYSTEM_INFO;
        drawSystemInfoScreen();
        playClickTone();
        return;
    }
    else if (key == 'D') {
        previousScreen = SCREEN_MENU;
        manualPage = 0;
        currentScreen = SCREEN_USER_MANUAL;
        drawUserManualScreen();
        playClickTone();
        return;
    }
    
    if (selectionChanged) {
        if (menuScrollOffset != oldScrollOffset) {
            drawMenuScreen();
        } else {
            updateMenuSelection(oldSelection, selectedAlertIndex);
        }
    }
}

void handleConfirmInput(char key) {
    if (key == '*') {
        currentScreen = SCREEN_SENDING;
        drawSendingScreen();
        
        lastTransmitSuccess = transmitAlert();
        
        retryCount = 0;
        currentScreen = SCREEN_RESULT;
        drawResultScreen();
    }
    else if (key == '#') {
        currentScreen = SCREEN_MENU;
        drawMenuScreen();
    }
}

void handleResultInput(char key) {
    if (lastTransmitSuccess) {
        clearAllLEDs();
        currentScreen = SCREEN_MENU;
        drawMenuScreen();
    } else {
        if (key == '*') {
            if (retryCount < MAX_RETRY_ATTEMPTS - 1) {
                retryCount++;
                clearAllLEDs();
                currentScreen = SCREEN_SENDING;
                drawSendingScreen();
                
                lastTransmitSuccess = transmitAlert();
                
                currentScreen = SCREEN_RESULT;
                drawResultScreen();
            }
        }
        else if (key == '#') {
            clearAllLEDs();
            retryCount = 0;
            currentScreen = SCREEN_MENU;
            drawMenuScreen();
        }
    }
}

void handleSystemInfoInput(char key) {
    currentScreen = SCREEN_MENU;
    drawMenuScreen();
}

void handleUserManualInput(char key) {
    if (key == 'A' && manualPage > 0) {
        manualPage--;
        drawUserManualScreen();
    }
    else if (key == 'B' && manualPage < MANUAL_TOTAL_PAGES - 1) {
        manualPage++;
        drawUserManualScreen();
    }
    else if (key == '#' || key == '*') {
        currentScreen = SCREEN_MENU;
        drawMenuScreen();
    }
}
