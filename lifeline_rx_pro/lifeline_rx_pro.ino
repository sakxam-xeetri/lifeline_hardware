/*
 * ═══════════════════════════════════════════════════════════════════════════════════
 *                        ╔═════════════════════════════════════╗
 *                        ║    LIFELINE EMERGENCY RECEIVER      ║
 *                        ║   Professional Base Station v3.1    ║
 *                        ║         (16x2 I2C LCD Edition)      ║
 *                        ╚═════════════════════════════════════╝
 * ═══════════════════════════════════════════════════════════════════════════════════
 * 
 * Production-ready emergency communication receiver designed for deployment in 
 * remote Himalayan and rural regions of Nepal. Operated at base stations for
 * monitoring emergency alerts from field transmitters.
 * 
 * DESIGN PRINCIPLES:
 *   ✓ Compact & High Readability - 16x2 Character LCD with custom glyphs
 *   ✓ High visibility alerts - Clear priority codes and text indicators
 *   ✓ Rugged base station appearance - Reliable government-grade emergency receiver
 * 
 * HARDWARE SPECIFICATION:
 *   - Microcontroller: ESP32 Development Board
 *   - Display: 16x2 Character LCD with I2C Module (SDA: GPIO 4, SCL: GPIO 16)
 *   - Communication: LoRa SX1278 @ 433 MHz
 *   - Indicators: Buzzer & Status LEDs for alert notifications
 * 
 * PACKET FORMAT: TX[ID],[ALERT_CODE] (e.g., "TX003,A")
 * 
 * Author: LifeLine Development Team
 * Version: 3.1.0 PRO
 * License: MIT
 * ═══════════════════════════════════════════════════════════════════════════════════
 */

#include <Wire.h>
#include <LoRa.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>

// ═══════════════════════════════════════════════════════════════════════════════════
//                              DEVICE CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════

#define DEVICE_ID           1              // Unique receiver ID (1-999)
#define DEVICE_NAME         "LifeLine RX"  // Device display name
#define FIRMWARE_VERSION    "v3.1.0 PRO"   // Firmware version string
#define DEVICE_TYPE         "RX"           // Device type identifier
#define LORA_FREQUENCY      433E6          // LoRa frequency in Hz (433 MHz)
#define LORA_SF             12             // LoRa Spreading Factor (7-12) - Max range
#define LORA_BW             125E3          // LoRa Bandwidth (125 kHz) - WORKING CONFIGURATION
#define LORA_CR             8              // LoRa Coding Rate 4/8 - Max error correction
#define LORA_PREAMBLE       16             // Preamble Length - Good for weak signals
#define LORA_SYNC_WORD      0x12           // LoRa Sync Word (default, must match TX)

// ═══════════════════════════════════════════════════════════════════════════════════
//                              WIFI & API CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════

// Web Dashboard API Endpoint
#define API_ENDPOINT        "https://zenithkandel.com.np/lifeline/API/Create/message.php"

// WiFi Portal Configuration
#define WIFI_AP_SSID        "LifeLine-RX-Setup"
#define WIFI_AP_PASSWORD    "lifeline123"
#define WIFI_PORTAL_TIMEOUT 180000         // Portal timeout: 3 minutes (ms)
#define WIFI_CONNECT_TIMEOUT 10000         // Connection timeout: 10 seconds (ms)

// ═══════════════════════════════════════════════════════════════════════════════════
//                              PIN DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════════════

// LoRa Module Pins (SX1278) - WORKING CONFIGURATION
#define LORA_CS     18      // Chip Select
#define LORA_RST    23      // Reset
#define LORA_DIO0   19      // Digital I/O 0 (Interrupt)

// SPI Bus Pins (Custom) - WORKING CONFIGURATION
#define SPI_SCK     5       // Serial Clock
#define SPI_MISO    17      // Master In Slave Out
#define SPI_MOSI    27      // Master Out Slave In

// 16x2 LCD Display Pins (I2C) - Reused from former TFT pins
#define LCD_SDA     4       // I2C Data (reused from TFT_DC)
#define LCD_SCL     16      // I2C Clock (reused from TFT_CS)
#define LCD_ADDR    0x27    // Default I2C Address for 1602 LCD (PCF8574)
#define LCD_COLS    16
#define LCD_ROWS    2

// LED Indicator Pins
#define LED_GREEN   21      // Success indicator (active HIGH)
#define LED_RED     13      // Alert indicator (active HIGH)

// Audio Feedback
#define BUZZER_PIN  12      // Buzzer

// WiFi Portal Trigger Button
#define WIFI_PORTAL_PIN  0  // EN/BOOT button (GPIO 0) - Press to open WiFi portal

// ═══════════════════════════════════════════════════════════════════════════════════
//                              LCD CUSTOM CHARACTERS (CGRAM 0-7)
// ═══════════════════════════════════════════════════════════════════════════════════

// Glyph 0: Radar/Antenna Icon
const uint8_t glyphRadar[8] = {
    0b00000,
    0b01110,
    0b10001,
    0b00100,
    0b01010,
    0b00000,
    0b00100,
    0b00000
};

// Glyph 1: Alert Bell Icon
const uint8_t glyphBell[8] = {
    0b00100,
    0b01110,
    0b01110,
    0b01110,
    0b11111,
    0b00000,
    0b00100,
    0b00000
};

// Glyph 2: Checkmark Icon
const uint8_t glyphCheck[8] = {
    0b00000,
    0b00001,
    0b00011,
    0b10110,
    0b11100,
    0b01000,
    0b00000,
    0b00000
};

// Glyph 3: Signal RSSI Icon
const uint8_t glyphSignal[8] = {
    0b00001,
    0b00001,
    0b00101,
    0b00101,
    0b10101,
    0b10101,
    0b00000,
    0b00000
};

// Glyph 4: Warning Triangle Icon
const uint8_t glyphWarn[8] = {
    0b00100,
    0b01110,
    0b11111,
    0b11011,
    0b11011,
    0b11111,
    0b00100,
    0b00000
};

// ═══════════════════════════════════════════════════════════════════════════════════
//                              ALERT DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════════════

#define ALERT_COUNT 15

// Alert messages - MUST match transmitter configuration
const char* alertNames[ALERT_COUNT] = {
    "EMERGENCY",            // 0  - A - CRITICAL
    "MEDICAL EMERGENCY",    // 1  - B - CRITICAL
    "MEDICINE SHORTAGE",    // 2  - C - MEDIUM
    "EVACUATION NEEDED",    // 3  - D - CRITICAL
    "STATUS OK",            // 4  - E - OK
    "INJURY REPORTED",      // 5  - F - HIGH
    "FOOD SHORTAGE",        // 6  - G - MEDIUM
    "WATER SHORTAGE",       // 7  - H - MEDIUM
    "WEATHER ALERT",        // 8  - I - MEDIUM
    "LOST PERSON",          // 9  - J - HIGH
    "ANIMAL ATTACK",        // 10 - K - HIGH
    "LANDSLIDE",            // 11 - L - HIGH
    "SNOW STORM",           // 12 - M - HIGH
    "EQUIPMENT FAILURE",    // 13 - N - MEDIUM
    "OTHER EMERGENCY"       // 14 - O - NEUTRAL
};

// Shorter names optimized for 16x2 LCD display (max 13 chars)
const char* alertNamesShort[ALERT_COUNT] = {
    "EMERGENCY",
    "MEDICAL EMERG",
    "MED SHORTAGE",
    "EVACUATION",
    "STATUS OK",
    "INJURY",
    "FOOD SHORT",
    "WATER SHORT",
    "WEATHER",
    "LOST PERSON",
    "ANIMAL ATTK",
    "LANDSLIDE",
    "SNOW STORM",
    "EQUIP FAIL",
    "OTHER"
};

// Priority levels: 0=CRITICAL, 1=HIGH, 2=MEDIUM, 3=OK, 4=NEUTRAL
const uint8_t alertPriority[ALERT_COUNT] = {
    0, 0, 2, 0, 3, 1, 2, 2, 2, 1, 1, 1, 1, 2, 4
};

// Priority label text for LCD
const char* priorityLabels[] = {"CRIT", "HIGH", "MED ", "OK  ", "INFO"};

// ═══════════════════════════════════════════════════════════════════════════════════
//                              TIMING CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════════

#define BOOT_DISPLAY_TIME       1000    // Boot screen duration (ms) - 1 second
#define ALERT_DISPLAY_TIME      30000   // Alert display time before auto-return (ms)
#define IDLE_PULSE_INTERVAL     600     // Pulse animation interval (ms)
#define HISTORY_MAX_ITEMS       10      // Maximum alerts in history

// ═══════════════════════════════════════════════════════════════════════════════════
//                              GLOBAL OBJECTS
// ═══════════════════════════════════════════════════════════════════════════════════

// 16x2 LCD Object Instance (I2C)
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// ═══════════════════════════════════════════════════════════════════════════════════
//                              STATE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════════════

enum ScreenState {
    SCREEN_BOOT,            // 0 - Initial boot screen
    SCREEN_IDLE,            // 1 - Waiting for signals
    SCREEN_ALERT,           // 2 - Alert display
    SCREEN_HISTORY,         // 3 - Alert history view
    SCREEN_SYSTEM_INFO      // 4 - System information display
};

ScreenState currentScreen = SCREEN_BOOT;

// Timing variables
unsigned long bootStartTime = 0;
unsigned long alertReceivedTime = 0;
unsigned long lastPulseTime = 0;
uint8_t pulseState = 0;
uint8_t bootDotState = 0;

// Current alert data
int lastDeviceId = 0;
int lastAlertIndex = 0;
int lastRssi = 0;

// Alert history structure
struct AlertRecord {
    int deviceId;
    int alertIndex;
    int rssi;
    unsigned long timestamp;
};

AlertRecord alertHistory[HISTORY_MAX_ITEMS];
int historyCount = 0;
int historyScrollOffset = 0;

// System status
bool loraInitialized = false;
int totalAlertsReceived = 0;

// ═══════════════════════════════════════════════════════════════════════════════════
//                              WIFI STATE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════════════

WebServer wifiServer(80);
Preferences preferences;

bool wifiConnected = false;
bool portalActive = false;
unsigned long portalStartTime = 0;
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
#define LONG_PRESS_DURATION 3000  // 3 seconds for long press

String storedSSID = "";
String storedPassword = "";

// Forward declarations
void drawWiFiPortalScreen();
void startWiFiPortal();
void stopWiFiPortal();
void printSerialDebugMenu();

// ═══════════════════════════════════════════════════════════════════════════════════
//                          SERIAL DEBUG CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════

#define SERIAL_DEBUG_ENABLED true
#define SERIAL_BAUD_RATE     115200

String serialInputBuffer = "";

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

// ═══════════════════════════════════════════════════════════════════════════════════
//                              UTILITY FUNCTIONS & LCD HELPERS
// ═══════════════════════════════════════════════════════════════════════════════════

char getAlertCode(int index) {
    if (index >= 0 && index < ALERT_COUNT) {
        return 'A' + index;
    }
    return 'X';
}

/**
 * Print line padded to exactly 16 characters on LCD row to avoid flicker
 */
void printLCDLine(uint8_t row, const String& text) {
    lcd.setCursor(0, row);
    String line = text;
    if (line.length() > 16) {
        line = line.substring(0, 16);
    }
    while (line.length() < 16) {
        line += " ";
    }
    lcd.print(line);
}

/**
 * Print text centered on specified LCD row
 */
void printLCDCenter(uint8_t row, const String& text) {
    String line = text;
    if (line.length() > 16) line = line.substring(0, 16);
    int spaces = (16 - line.length()) / 2;
    String fullLine = "";
    for (int i = 0; i < spaces; i++) fullLine += " ";
    fullLine += line;
    while (fullLine.length() < 16) fullLine += " ";
    lcd.setCursor(0, row);
    lcd.print(fullLine);
}

/**
 * Audio feedback functions
 */
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

void playBootTone() {
    tone(BUZZER_PIN, 1500, 80);
}

void addToHistory(int deviceId, int alertIndex, int rssi) {
    if (historyCount >= HISTORY_MAX_ITEMS) {
        for (int i = 0; i < HISTORY_MAX_ITEMS - 1; i++) {
            alertHistory[i] = alertHistory[i + 1];
        }
        historyCount = HISTORY_MAX_ITEMS - 1;
    }
    
    alertHistory[historyCount].deviceId = deviceId;
    alertHistory[historyCount].alertIndex = alertIndex;
    alertHistory[historyCount].rssi = rssi;
    alertHistory[historyCount].timestamp = millis();
    historyCount++;
    totalAlertsReceived++;
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              1. BOOT SCREEN
// ═══════════════════════════════════════════════════════════════════════════════════

void drawBootScreen() {
    lcd.clear();
    printLCDCenter(0, "LIFELINE RX v3.1");
    printLCDCenter(1, "Init LoRa...");
    bootStartTime = millis();
    bootDotState = 0;
    Serial.println(F("[SCREEN] Boot screen displayed on 16x2 LCD"));
}

bool updateBootAnimation() {
    unsigned long elapsed = millis() - bootStartTime;
    uint8_t newDotState = (elapsed / 300) % 4;
    
    if (newDotState != bootDotState) {
        bootDotState = newDotState;
        String status = "Ready";
        for (int i = 0; i < bootDotState; i++) {
            status += ".";
        }
        printLCDCenter(1, status);
    }
    
    return elapsed >= BOOT_DISPLAY_TIME;
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              2. IDLE SCREEN
// ═══════════════════════════════════════════════════════════════════════════════════

void drawIdleScreen() {
    // Line 0: Title + Radar Icon
    lcd.setCursor(0, 0);
    lcd.print("LIFELINE RX  ");
    lcd.write(0); // Radar symbol
    lcd.print(" ");
    
    // Line 1: Freq & Alerts count
    char line1[17];
    snprintf(line1, sizeof(line1), "433MHz  Alt:%-4d", totalAlertsReceived);
    printLCDLine(1, line1);
    
    lastPulseTime = millis();
    pulseState = 0;
    Serial.println(F("[SCREEN] Idle screen displayed on 16x2 LCD"));
}

void updateIdleAnimation() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastPulseTime >= IDLE_PULSE_INTERVAL) {
        lastPulseTime = currentTime;
        
        // Toggle radar icon at column 14, row 0
        lcd.setCursor(14, 0);
        if (pulseState == 0) {
            lcd.write(0); // Radar glyph
        } else {
            lcd.print("o");
        }
        pulseState = (pulseState + 1) % 2;
        
        // Update total alert count dynamically on line 1
        char line1[17];
        snprintf(line1, sizeof(line1), "433MHz  Alt:%-4d", totalAlertsReceived);
        printLCDLine(1, line1);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              3. ALERT SCREEN
// ═══════════════════════════════════════════════════════════════════════════════════

void drawAlertScreen(int deviceId, int alertIndex, int rssi) {
    uint8_t priority = alertPriority[alertIndex];
    char code = getAlertCode(alertIndex);
    const char* nameShort = alertNamesShort[alertIndex];
    
    // Line 0: Warning icon + Code + Short Name (e.g. "!A MEDICAL EMERG")
    lcd.setCursor(0, 0);
    lcd.write(4); // Warning icon glyph
    
    char row0[17];
    snprintf(row0, sizeof(row0), "%c %-13s", code, nameShort);
    lcd.print(row0);
    
    // Line 1: Device ID + RSSI + Priority (e.g. "TX#001 -65dBm CRIT")
    const char* priLabel = (priority == 0) ? "CRIT" : (priority == 1) ? "HIGH" : (priority == 2) ? "MED " : "OK  ";
    
    char row1[17];
    snprintf(row1, sizeof(row1), "TX#%03d %d%s %s", deviceId % 1000, rssi, "dBm", priLabel);
    printLCDLine(1, row1);
    
    // Store alert data
    lastDeviceId = deviceId;
    lastAlertIndex = alertIndex;
    lastRssi = rssi;
    alertReceivedTime = millis();
    
    // Add to history
    addToHistory(deviceId, alertIndex, rssi);
    
    // Play alert sound
    playAlertTone(priority);
    
    Serial.printf("[SCREEN] Alert displayed on LCD: Device %d, Alert %d (%s)\n", 
                  deviceId, alertIndex, alertNames[alertIndex]);
}

bool shouldReturnToIdle() {
    if (currentScreen != SCREEN_ALERT) return false;
    return (millis() - alertReceivedTime >= ALERT_DISPLAY_TIME);
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              LORA PACKET HANDLING
// ═══════════════════════════════════════════════════════════════════════════════════

bool parseLoRaPacket(int& deviceId, int& alertIndex, int& rssi) {
    int packetSize = LoRa.parsePacket();
    if (packetSize == 0) return false;
    
    String data = "";
    while (LoRa.available()) {
        data += (char)LoRa.read();
    }
    
    rssi = LoRa.packetRssi();
    
    Serial.printf("[RX] Raw packet (%d bytes): '%s', RSSI: %d\n", packetSize, data.c_str(), rssi);
    
    if (data.startsWith("TX")) {
        data = data.substring(2);
    }
    
    int comma = data.indexOf(',');
    if (comma <= 0) {
        Serial.println(F("[RX] Invalid packet format"));
        return false;
    }
    
    deviceId = data.substring(0, comma).toInt();
    
    String alertPart = data.substring(comma + 1);
    alertPart.trim();
    
    if (alertPart.length() == 1 && alertPart[0] >= 'A' && alertPart[0] <= 'O') {
        alertIndex = alertPart[0] - 'A';
    } else if (alertPart.length() == 1 && alertPart[0] >= 'a' && alertPart[0] <= 'o') {
        alertIndex = alertPart[0] - 'a';
    } else {
        alertIndex = alertPart.toInt();
    }
    
    if (alertIndex < 0 || alertIndex >= ALERT_COUNT) {
        Serial.printf("[RX] Invalid alert index %d, defaulting to OTHER\n", alertIndex);
        alertIndex = ALERT_COUNT - 1;
    }
    
    Serial.printf("[RX] Parsed: Device=%d, Alert=%d (%s)\n", deviceId, alertIndex, alertNames[alertIndex]);
    
    LoRa.receive();
    
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              WIFI & API FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════════

void loadWiFiCredentials() {
    preferences.begin("lifeline", true);
    storedSSID = preferences.getString("ssid", "");
    storedPassword = preferences.getString("password", "");
    preferences.end();
    
    if (storedSSID.length() > 0) {
        Serial.printf("[WIFI] Loaded credentials for SSID: %s\n", storedSSID.c_str());
    } else {
        Serial.println(F("[WIFI] No stored credentials found"));
    }
}

void saveWiFiCredentials(const String& ssid, const String& password) {
    preferences.begin("lifeline", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    
    storedSSID = ssid;
    storedPassword = password;
    Serial.printf("[WIFI] Saved credentials for SSID: %s\n", ssid.c_str());
}

void drawWiFiConnectingScreen(const String& ssid) {
    printLCDLine(0, "WiFi Connecting");
    String line1 = "SSID:" + ssid;
    printLCDLine(1, line1);
}

void drawWiFiConnectedScreen(const String& ip) {
    printLCDLine(0, "WiFi Connected!");
    String line1 = "IP:" + ip;
    printLCDLine(1, line1);
}

void drawWiFiFailedScreen() {
    printLCDLine(0, "WiFi Failed!");
    printLCDLine(1, "Opening Portal..");
}

bool connectToWiFiSilent() {
    if (storedSSID.length() == 0) {
        Serial.println(F("[WIFI] No credentials stored"));
        return false;
    }
    
    Serial.printf("[WIFI] Silent connecting to %s...\n", storedSSID.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(storedSSID.c_str(), storedPassword.c_str());
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.printf("[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        wifiConnected = false;
        Serial.println(F("[WIFI] Connection failed - Hold EN 3s to reconfigure"));
        return false;
    }
}

bool connectToWiFi() {
    if (storedSSID.length() == 0) {
        Serial.println(F("[WIFI] No credentials stored"));
        return false;
    }
    
    Serial.printf("[WIFI] Connecting to %s...\n", storedSSID.c_str());
    
    drawWiFiConnectingScreen(storedSSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(storedSSID.c_str(), storedPassword.c_str());
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        String ip = WiFi.localIP().toString();
        Serial.printf("[WIFI] Connected! IP: %s\n", ip.c_str());
        
        drawWiFiConnectedScreen(ip);
        delay(2000);
        return true;
    } else {
        wifiConnected = false;
        Serial.println(F("[WIFI] Connection failed"));
        
        drawWiFiFailedScreen();
        delay(1500);
        
        startWiFiPortal();
        return false;
    }
}

void pushAlertToAPI(int deviceId, int alertIndex, int rssi) {
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[API] WiFi not connected, skipping API push"));
        return;
    }
    
    HTTPClient http;
    http.begin(API_ENDPOINT);
    http.addHeader("Content-Type", "application/json");
    
    String jsonPayload = "{\"DID\":" + String(deviceId) + 
                         ",\"message_code\":" + String(alertIndex) + 
                         ",\"RSSI\":" + String(rssi) + "}";
    
    Serial.printf("[API] Sending: %s\n", jsonPayload.c_str());
    
    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("[API] Response (%d): %s\n", httpResponseCode, response.c_str());
    } else {
        Serial.printf("[API] Error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
}

String getPortalHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LifeLine RX WiFi Setup</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;background:#1a1a2e;color:#fff;margin:0;padding:20px;}";
    html += ".container{max-width:400px;margin:0 auto;background:#252542;padding:30px;border-radius:10px;}";
    html += "h1{color:#00d4ff;text-align:center;margin-bottom:30px;}";
    html += "h2{color:#ffb800;font-size:16px;margin-bottom:20px;}";
    html += "input[type=text],input[type=password]{width:100%;padding:12px;margin:8px 0 20px;border:1px solid #444;border-radius:5px;background:#1a1a2e;color:#fff;box-sizing:border-box;}";
    html += "input[type=submit]{width:100%;padding:14px;background:#00d4ff;color:#000;border:none;border-radius:5px;cursor:pointer;font-weight:bold;font-size:16px;}";
    html += "input[type=submit]:hover{background:#00b8e6;}";
    html += ".status{text-align:center;margin-top:20px;padding:10px;border-radius:5px;}";
    html += ".success{background:#00ff87;color:#000;}";
    html += ".info{background:#2d2d44;color:#a3b1c6;}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>LifeLine RX</h1>";
    html += "<h2>WiFi Configuration</h2>";
    html += "<form action='/save' method='POST'>";
    html += "<label>WiFi Network Name (SSID):</label>";
    html += "<input type='text' name='ssid' placeholder='Enter WiFi name' required>";
    html += "<label>WiFi Password:</label>";
    html += "<input type='password' name='password' placeholder='Enter password'>";
    html += "<input type='submit' value='Connect'>";
    html += "</form>";
    
    if (storedSSID.length() > 0) {
        html += "<div class='status info'>Currently configured: " + storedSSID + "</div>";
    }
    
    html += "</div></body></html>";
    return html;
}

void handlePortalRoot() {
    wifiServer.send(200, "text/html", getPortalHTML());
}

void handlePortalSave() {
    String ssid = wifiServer.arg("ssid");
    String password = wifiServer.arg("password");
    
    if (ssid.length() > 0) {
        saveWiFiCredentials(ssid, password);
        
        String html = "<!DOCTYPE html><html><head>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<title>LifeLine RX - Saved</title>";
        html += "<style>body{font-family:Arial;background:#1a1a2e;color:#fff;text-align:center;padding:50px;}";
        html += ".success{background:#00ff87;color:#000;padding:20px;border-radius:10px;max-width:400px;margin:0 auto;}</style></head><body>";
        html += "<div class='success'><h2>WiFi Credentials Saved!</h2>";
        html += "<p>SSID: " + ssid + "</p>";
        html += "<p>Device will restart and connect to WiFi...</p></div>";
        html += "</body></html>";
        
        wifiServer.send(200, "text/html", html);
        
        delay(2000);
        ESP.restart();
    } else {
        wifiServer.send(400, "text/plain", "SSID is required");
    }
}

void startWiFiPortal() {
    Serial.println(F("[WIFI] Starting configuration portal..."));
    
    WiFi.disconnect(true);
    delay(100);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[WIFI] Portal started! Connect to '%s' (password: %s)\n", WIFI_AP_SSID, WIFI_AP_PASSWORD);
    Serial.printf("[WIFI] Portal IP: %s\n", apIP.toString().c_str());
    
    wifiServer.on("/", handlePortalRoot);
    wifiServer.on("/save", HTTP_POST, handlePortalSave);
    wifiServer.begin();
    
    portalActive = true;
    portalStartTime = millis();
    
    drawWiFiPortalScreen();
}

void stopWiFiPortal() {
    if (!portalActive) return;
    
    Serial.println(F("[WIFI] Stopping portal..."));
    wifiServer.stop();
    WiFi.softAPdisconnect(true);
    portalActive = false;
    
    if (storedSSID.length() > 0) {
        connectToWiFi();
    }
    
    if (currentScreen != SCREEN_ALERT) {
        currentScreen = SCREEN_IDLE;
        drawIdleScreen();
    }
}

void drawWiFiPortalScreen() {
    printLCDLine(0, "AP:LifeLine-RX");
    printLCDLine(1, "IP:192.168.4.1");
}

void drawLongPressProgress(float progress) {
    printLCDLine(0, "WiFi Setup Mode");
    int fillCount = (int)(progress * 10.0);
    if (fillCount > 10) fillCount = 10;
    
    String bar = "Hold:[";
    for (int i = 0; i < 10; i++) {
        if (i < fillCount) bar += "=";
        else bar += " ";
    }
    bar += "]";
    printLCDLine(1, bar);
}

void checkWiFiPortalButton() {
    bool currentButtonState = digitalRead(WIFI_PORTAL_PIN);
    
    if (currentButtonState == LOW) {
        if (!buttonPressed) {
            buttonPressed = true;
            buttonPressStartTime = millis();
        } else {
            unsigned long pressDuration = millis() - buttonPressStartTime;
            float progress = (float)pressDuration / LONG_PRESS_DURATION;
            
            if (progress > 1.0) progress = 1.0;
            
            if (!portalActive && pressDuration > 200) {
                drawLongPressProgress(progress);
            }
            
            if (pressDuration >= LONG_PRESS_DURATION) {
                buttonPressed = false;
                
                if (portalActive) {
                    stopWiFiPortal();
                } else {
                    if (storedSSID.length() > 0) {
                        connectToWiFi();
                    } else {
                        startWiFiPortal();
                    }
                }
                
                if (!portalActive) {
                    if (currentScreen == SCREEN_IDLE) {
                        drawIdleScreen();
                    } else if (currentScreen == SCREEN_ALERT) {
                        drawAlertScreen(lastDeviceId, lastAlertIndex, lastRssi);
                    }
                }
            }
        }
    } else {
        if (buttonPressed) {
            buttonPressed = false;
            
            if (!portalActive && (millis() - buttonPressStartTime) > 200) {
                if (currentScreen == SCREEN_IDLE) {
                    drawIdleScreen();
                } else if (currentScreen == SCREEN_ALERT) {
                    drawAlertScreen(lastDeviceId, lastAlertIndex, lastRssi);
                }
            }
        }
    }
}

void handleWiFiPortal() {
    if (!portalActive) return;
    
    wifiServer.handleClient();
    
    if (millis() - portalStartTime >= WIFI_PORTAL_TIMEOUT) {
        Serial.println(F("[WIFI] Portal timeout, closing..."));
        stopWiFiPortal();
    }
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              SETUP
// ═══════════════════════════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
    
    Serial.println(F("\n╔═══════════════════════════════════════════════════════════╗"));
    Serial.println(F("║      LIFELINE EMERGENCY RECEIVER v3.1 PRO                 ║"));
    Serial.println(F("║          16x2 I2C LCD Base Station                        ║"));
    Serial.println(F("╚═══════════════════════════════════════════════════════════╝\n"));
    
    // Initialize pins
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LORA_CS, OUTPUT);
    pinMode(LORA_RST, OUTPUT);
    pinMode(WIFI_PORTAL_PIN, INPUT_PULLUP);
    
    digitalWrite(LORA_CS, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    
    // Manual LoRa reset for reliable initialization
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);
    
    // Initialize I2C Bus for LCD (SDA: GPIO 4, SCL: GPIO 16)
    Wire.begin(LCD_SDA, LCD_SCL);
    
    // Initialize 16x2 I2C LCD
    lcd.init();
    lcd.backlight();
    
    // Register custom glyphs into LCD CGRAM (0-4)
    lcd.createChar(0, (uint8_t*)glyphRadar);
    lcd.createChar(1, (uint8_t*)glyphBell);
    lcd.createChar(2, (uint8_t*)glyphCheck);
    lcd.createChar(3, (uint8_t*)glyphSignal);
    lcd.createChar(4, (uint8_t*)glyphWarn);
    
    Serial.printf("[OK] 16x2 I2C LCD initialized (SDA: GPIO %d, SCL: GPIO %d, Addr: 0x%02X)\n", 
                  LCD_SDA, LCD_SCL, LCD_ADDR);
    
    // Initialize SPI for LoRa
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    Serial.println(F("[OK] SPI initialized for LoRa"));
    
    // Initialize LoRa
    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(LORA_FREQUENCY)) {
        Serial.println(F("[ERROR] LoRa initialization failed!"));
        loraInitialized = false;
    } else {
        LoRa.setSpreadingFactor(LORA_SF);
        LoRa.setSignalBandwidth(LORA_BW);
        LoRa.enableCrc();
        loraInitialized = true;
        Serial.println(F("[OK] LoRa initialized @ 433MHz, SF12, BW125kHz, CRC enabled"));
        Serial.println(F("[OK] LoRa in continuous receive mode"));
    }
    
    // Show boot screen on LCD
    currentScreen = SCREEN_BOOT;
    drawBootScreen();
    playBootTone();
    
    Serial.println(F("[OK] Boot screen displayed"));
    
    // Load and auto-connect WiFi if credentials exist
    loadWiFiCredentials();
    if (storedSSID.length() > 0) {
        Serial.printf("[WIFI] Auto-connecting to: %s\n", storedSSID.c_str());
        connectToWiFiSilent();
    } else {
        Serial.println(F("[WIFI] No credentials - Hold EN button for 3 seconds to configure"));
    }
    
    Serial.println(F("=== Ready to receive emergency alerts ===\n"));
    
    #if SERIAL_DEBUG_ENABLED
    printSerialDebugMenu();
    #endif
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              MAIN LOOP
// ═══════════════════════════════════════════════════════════════════════════════════

void loop() {
    if (portalActive) {
        checkWiFiPortalButton();
        handleWiFiPortal();
        return;
    }
    
    switch (currentScreen) {
        
        case SCREEN_BOOT:
            if (updateBootAnimation()) {
                currentScreen = SCREEN_IDLE;
                drawIdleScreen();
                digitalWrite(LED_GREEN, HIGH);
                Serial.println(F("[STATE] Switched to IDLE - Listening for alerts"));
            }
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
                    
                    pushAlertToAPI(deviceId, alertIndex, rssi);
                    
                    currentScreen = SCREEN_ALERT;
                    drawAlertScreen(deviceId, alertIndex, rssi);
                    
                    if (alertPriority[alertIndex] <= 1) {
                        digitalWrite(LED_RED, HIGH);
                        digitalWrite(LED_GREEN, LOW);
                    } else {
                        digitalWrite(LED_GREEN, HIGH);
                        digitalWrite(LED_RED, LOW);
                    }
                    
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
                    
                    pushAlertToAPI(deviceId, alertIndex, rssi);
                    
                    drawAlertScreen(deviceId, alertIndex, rssi);
                    
                    if (alertPriority[alertIndex] <= 1) {
                        digitalWrite(LED_RED, HIGH);
                        digitalWrite(LED_GREEN, LOW);
                    }
                }
            }
            
            if (shouldReturnToIdle()) {
                currentScreen = SCREEN_IDLE;
                drawIdleScreen();
                digitalWrite(LED_GREEN, HIGH);
                digitalWrite(LED_RED, LOW);
                Serial.println(F("[STATE] Auto-returned to IDLE"));
            }
            break;
            
        case SCREEN_HISTORY:
        case SCREEN_SYSTEM_INFO:
            break;
    }
    
    delay(10);
}
