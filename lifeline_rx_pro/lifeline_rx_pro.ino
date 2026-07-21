/*
 * ═══════════════════════════════════════════════════════════════════════════════════
 *                        ╔═════════════════════════════════════╗
 *                        ║    LIFELINE EMERGENCY RECEIVER      ║
 *                        ║   Professional Base Station v3.1    ║
 *                        ║ (16x2 LCD, Multi-WiFi & OTA Web)    ║
 *                        ╚═════════════════════════════════════╝
 * ═══════════════════════════════════════════════════════════════════════════════════
 */

#include "Config.h"
#include "BuzzerLED.h"
#include "DisplayUI.h"
#include "LoRaComm.h"
#include "WiFiPortal.h"
#include "APIClient.h"

// Static serial debugger variables
static String serialInputBuffer = "";

// Forward declarations for Serial Debug Menu
void printSerialDebugMenu();
bool checkSerialSimulatedPacket(int& deviceId, int& alertIndex, int& rssi);

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
    
    Serial.println(F("\n╔═══════════════════════════════════════════════════════════╗"));
    Serial.println(F("║      LIFELINE EMERGENCY RECEIVER v3.1 PRO                 ║"));
    Serial.println(F("║     16x2 LCD & Multi-WiFi (WiFi Button + OTA Web)         ║"));
    Serial.println(F("╚═══════════════════════════════════════════════════════════╝\n"));
    
    // Initialize IOs
    initBuzzerLED();
    
    pinMode(LORA_CS, OUTPUT);
    pinMode(LORA_RST, OUTPUT);
    pinMode(WIFI_PORTAL_PIN, INPUT_PULLUP);
    
    digitalWrite(LORA_CS, HIGH);
    
    // LoRa module hardware reset
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);
    
    // Setup sub-systems
    initDisplay();
    
    currentScreen = SCREEN_BOOT;
    drawBootScreen();
    playBootTone();
    
    Serial.println(F("[OK] Boot screen displayed"));
    
    initLoRa();
    
    Serial.println(F("=== Ready to receive emergency alerts ===\n"));
    
    #if SERIAL_DEBUG_ENABLED
    printSerialDebugMenu();
    #endif
}

void loop() {
    updateLEDs();
    
    if (portalActive) {
        checkWiFiPortalButton();
        handleWiFiPortal();
        return;
    }
    
    switch (currentScreen) {
        
        case SCREEN_BOOT:
            if (updateBootAnimation()) {
                loadWiFiCredentials();
                if (networkCount > 0) {
                    Serial.printf("[WIFI] Auto-connecting to %d stored network(s). Primary: %s\n", networkCount, activeSSID.c_str());
                    bool connected = connectToWiFi();
                    if (connected) {
                        currentScreen = SCREEN_IDLE;
                        drawIdleScreen();
                        Serial.println(F("[STATE] WiFi connected -> Switched to IDLE"));
                    } else {
                        currentScreen = SCREEN_NO_WIFI;
                        drawNoWiFiScreen();
                        Serial.println(F("[STATE] WiFi failed -> Switched to NO_WIFI screen"));
                    }
                } else {
                    Serial.println(F("[WIFI] No stored credentials -> Switched to NO_WIFI screen"));
                    playWiFiFailTone();
                    currentScreen = SCREEN_NO_WIFI;
                    drawNoWiFiScreen();
                }
            }
            break;
            
        case SCREEN_NO_WIFI:
            checkWiFiPortalButton();
            
            // Auto timeout 60 seconds -> switch to IDLE
            if (millis() - noWiFiStartTime >= NO_WIFI_TIMEOUT) {
                playReturnIdleTone();
                currentScreen = SCREEN_IDLE;
                drawIdleScreen();
                Serial.println(F("[STATE] No WiFi screen timeout (60s) -> Switched to IDLE"));
            }
            
            // Check for incoming emergency alerts even when on No WiFi screen
            {
                int deviceId, alertIndex, rssi;
                bool packetReceived = parseLoRaPacket(deviceId, alertIndex, rssi);
                
                #if SERIAL_DEBUG_ENABLED
                if (!packetReceived) {
                    packetReceived = checkSerialSimulatedPacket(deviceId, alertIndex, rssi);
                }
                #endif
                
                if (packetReceived) {
                    Serial.printf("[RX] Alert received: Device=%d, Alert=%d (%s), RSSI=%d\n",
                                  deviceId, alertIndex, alertNames[alertIndex], rssi);
                    
                    triggerRxBlink();
                    pushAlertToAPI(deviceId, alertIndex, rssi);
                    
                    currentScreen = SCREEN_ALERT;
                    drawAlertScreen(deviceId, alertIndex, rssi);
                    Serial.println(F("[STATE] Switched to ALERT from NO_WIFI screen"));
                }
            }
            break;
            
        case SCREEN_COUNTDOWN:
            checkWiFiPortalButton();
            break;
            
        case SCREEN_PORTAL:
            checkWiFiPortalButton();
            handleWiFiPortal();
            break;
            
        case SCREEN_IDLE:
            checkWiFiPortalButton();
            updateIdleAnimation();
            
            {
                int deviceId, alertIndex, rssi;
                bool packetReceived = parseLoRaPacket(deviceId, alertIndex, rssi);
                
                #if SERIAL_DEBUG_ENABLED
                if (!packetReceived) {
                    packetReceived = checkSerialSimulatedPacket(deviceId, alertIndex, rssi);
                }
                #endif
                
                if (packetReceived) {
                    Serial.printf("[RX] Alert received: Device=%d, Alert=%d (%s), RSSI=%d\n",
                                  deviceId, alertIndex, alertNames[alertIndex], rssi);
                    
                    triggerRxBlink();
                    pushAlertToAPI(deviceId, alertIndex, rssi);
                    
                    currentScreen = SCREEN_ALERT;
                    drawAlertScreen(deviceId, alertIndex, rssi);
                    
                    Serial.println(F("[STATE] Switched to ALERT"));
                }
            }
            break;
            
        case SCREEN_ALERT:
            checkWiFiPortalButton();
            
            {
                int deviceId, alertIndex, rssi;
                bool packetReceived = parseLoRaPacket(deviceId, alertIndex, rssi);
                
                #if SERIAL_DEBUG_ENABLED
                if (!packetReceived) {
                    packetReceived = checkSerialSimulatedPacket(deviceId, alertIndex, rssi);
                }
                #endif
                
                if (packetReceived) {
                    Serial.printf("[RX] New alert received while displaying: Device=%d, Alert=%d\n", 
                                  deviceId, alertIndex);
                    
                    triggerRxBlink();
                    pushAlertToAPI(deviceId, alertIndex, rssi);
                    
                    drawAlertScreen(deviceId, alertIndex, rssi);
                }
            }
            
            if (shouldReturnToIdle()) {
                playReturnIdleTone();
                currentScreen = SCREEN_IDLE;
                drawIdleScreen();
                Serial.println(F("[STATE] Auto-returned to IDLE from ALERT (15s timeout)"));
            }
            break;
            
        case SCREEN_WIFI_SPLASH:
        case SCREEN_HISTORY:
        case SCREEN_SYSTEM_INFO:
            break;
    }
    
    delay(10);
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                          SERIAL DEBUG CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════
bool checkSerialSimulatedPacket(int& deviceId, int& alertIndex, int& rssi) {
    if (!Serial.available()) return false;
    
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (serialInputBuffer.length() > 0) break;
        } else {
            serialInputBuffer += c;
        }
    }
    
    if (serialInputBuffer.length() == 0) return false;
    
    String input = serialInputBuffer;
    serialInputBuffer = "";
    input.trim();
    
    if (input.length() == 0) return false;
    
    if (input == "h" || input == "H" || input == "help" || input == "?") {
        printSerialDebugMenu();
        return false;
    }
    
    if (input.length() == 1 && ((input[0] >= '0' && input[0] <= '9'))) {
        deviceId = 1;
        alertIndex = (input[0] == '0') ? 9 : input[0] - '1';
        rssi = -65;
        Serial.printf("[SERIAL DEBUG] Quick alert: Device=%d, Alert=%d (%s)\n", 
                      deviceId, alertIndex, alertNames[alertIndex]);
        return true;
    }
    
    if (input.length() == 1 && ((input[0] >= 'A' && input[0] <= 'O') || (input[0] >= 'a' && input[0] <= 'o'))) {
        deviceId = 1;
        char code = (input[0] >= 'a') ? (input[0] - 'a' + 'A') : input[0];
        alertIndex = code - 'A';
        rssi = -65;
        Serial.printf("[SERIAL DEBUG] Quick alert: Device=%d, Alert=%c (%s)\n", 
                      deviceId, code, alertNames[alertIndex]);
        return true;
    }
    
    int comma = input.indexOf(',');
    if (comma <= 0) {
        Serial.println("[SERIAL DEBUG] Invalid format. Use: DEVICE_ID,ALERT_CODE (e.g., '3,A')");
        return false;
    }
    
    deviceId = input.substring(0, comma).toInt();
    String alertPart = input.substring(comma + 1);
    alertPart.trim();
    
    if (alertPart.length() == 1 && alertPart[0] >= 'A' && alertPart[0] <= 'O') {
        alertIndex = alertPart[0] - 'A';
    } else if (alertPart.length() == 1 && alertPart[0] >= 'a' && alertPart[0] <= 'o') {
        alertIndex = alertPart[0] - 'a';
    } else {
        alertIndex = alertPart.toInt();
    }
    
    if (alertIndex < 0 || alertIndex >= ALERT_COUNT) {
        Serial.printf("[SERIAL DEBUG] Invalid alert index: %d\n", alertIndex);
        return false;
    }
    
    rssi = -65;
    Serial.printf("[SERIAL DEBUG] Simulated packet: Device=%d, Alert=%d (%s)\n", 
                  deviceId, alertIndex, alertNames[alertIndex]);
    
    return true;
}

void printSerialDebugMenu() {
    Serial.println(F("\n╔════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║       LIFELINE RECEIVER v3.1 PRO - Serial Debug            ║"));
    Serial.println(F("╠════════════════════════════════════════════════════════════╣"));
    Serial.println(F("║ QUICK COMMANDS:                                            ║"));
    Serial.println(F("║   1-9  : Simulate alert 1-9 from device 1                  ║"));
    Serial.println(F("║   0    : Simulate alert 10 from device 1                   ║"));
    Serial.println(F("║   A-O  : Simulate alert by code from device 1              ║"));
    Serial.println(F("║                                                            ║"));
    Serial.println(F("║ FULL FORMAT:                                               ║"));
    Serial.println(F("║   DEVICE_ID,ALERT_CODE  (e.g., '3,A' or '3,5')             ║"));
    Serial.println(F("║                                                            ║"));
    Serial.println(F("║ ALERT CODES:                                               ║"));
    Serial.println(F("║   A(0)=EMERGENCY       B(1)=MEDICAL      C(2)=MEDICINE     ║"));
    Serial.println(F("║   D(3)=EVACUATION      E(4)=STATUS OK    F(5)=INJURY       ║"));
    Serial.println(F("║   G(6)=FOOD            H(7)=WATER        I(8)=WEATHER      ║"));
    Serial.println(F("║   J(9)=LOST PERSON     K(10)=ANIMAL      L(11)=LANDSLIDE   ║"));
    Serial.println(F("║   M(12)=SNOW STORM     N(13)=EQUIPMENT   O(14)=OTHER       ║"));
    Serial.println(F("╚════════════════════════════════════════════════════════════╝\n"));
}
