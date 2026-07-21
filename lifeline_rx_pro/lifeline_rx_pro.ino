/*
 * ═══════════════════════════════════════════════════════════════════════════════════
 *                        ╔═════════════════════════════════════╗
 *                        ║    LIFELINE EMERGENCY RECEIVER      ║
 *                        ║   Professional Base Station v3.1    ║
 *                        ║ (16x2 LCD, Multi-WiFi & OTA Web)    ║
 *                        ╚═════════════════════════════════════╝
 * ═══════════════════════════════════════════════════════════════════════════════════
 * 
 * Production-ready emergency communication receiver designed for deployment in 
 * remote Himalayan and rural regions of Nepal. Operated at base stations for
 * monitoring emergency alerts from field transmitters.
 * 
 * DESIGN PRINCIPLES:
 *   ✓ Dedicated Setup Button - Momentary push button on GPIO 14 (Hold 3s)
 *   ✓ Web OTA Firmware Update - Flash binary updates over WiFi at /update
 *   ✓ Multi-WiFi Support - Store up to 3 networks with automatic failover
 *   ✓ Free Setup AP - Passwordless open AP portal for quick setup
 * 
 * HARDWARE SPECIFICATION:
 *   - Microcontroller: ESP32 Development Board
 *   - Display: 16x2 Character LCD with I2C Module (SDA: GPIO 4, SCL: GPIO 16)
 *   - Communication: LoRa SX1278 @ 433 MHz
 *   - Indicators: Buzzer & Status LEDs for alert notifications
 *   - Setup Button: GPIO 14 (Momentary Push Button held for 3 seconds)
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
#include <Update.h>

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

// WiFi Portal Configuration (Open network - no password needed)
#define WIFI_AP_SSID        "LifeLine-RX-Setup"
#define WIFI_PORTAL_TIMEOUT 180000         // Portal timeout: 3 minutes (ms)
#define WIFI_CONNECT_TIMEOUT 8000          // Connection timeout per network: 8 seconds (ms)
#define MAX_WIFI_NETWORKS   3              // Up to 3 stored WiFi networks

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

// Dedicated WiFi Portal Push Button
#define WIFI_PORTAL_PIN  14 // Push button on GPIO 14 (Active LOW with internal pullup)

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

const uint8_t alertPriority[ALERT_COUNT] = {
    0, 0, 2, 0, 3, 1, 2, 2, 2, 1, 1, 1, 1, 2, 4
};

const char* priorityLabels[] = {"CRIT", "HIGH", "MED ", "OK  ", "INFO"};

// ═══════════════════════════════════════════════════════════════════════════════════
//                              TIMING CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════════

#define BOOT_DISPLAY_TIME       1000    // Boot screen duration (ms)
#define ALERT_DISPLAY_TIME      30000   // Alert display time before auto-return (ms)
#define IDLE_PULSE_INTERVAL     600     // Pulse animation interval (ms)
#define HISTORY_MAX_ITEMS       10      // Maximum alerts in history

// ═══════════════════════════════════════════════════════════════════════════════════
//                              GLOBAL OBJECTS & MULTI-WIFI
// ═══════════════════════════════════════════════════════════════════════════════════

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

struct WiFiNetwork {
    String ssid;
    String password;
};

WiFiNetwork storedNetworks[MAX_WIFI_NETWORKS];
int networkCount = 0;
String activeSSID = "";

// ═══════════════════════════════════════════════════════════════════════════════════
//                              STATE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════════════

enum ScreenState {
    SCREEN_BOOT,            // 0 - Boot screen
    SCREEN_IDLE,            // 1 - Listening state
    SCREEN_ALERT,           // 2 - Alert display
    SCREEN_HISTORY,         // 3 - History
    SCREEN_SYSTEM_INFO      // 4 - System info
};

ScreenState currentScreen = SCREEN_BOOT;

unsigned long bootStartTime = 0;
unsigned long alertReceivedTime = 0;
unsigned long lastPulseTime = 0;
uint8_t pulseState = 0;
uint8_t bootDotState = 0;

int lastDeviceId = 0;
int lastAlertIndex = 0;
int lastRssi = 0;

struct AlertRecord {
    int deviceId;
    int alertIndex;
    int rssi;
    unsigned long timestamp;
};

AlertRecord alertHistory[HISTORY_MAX_ITEMS];
int historyCount = 0;
int historyScrollOffset = 0;

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
int lastBeepSecond = -1;
#define LONG_PRESS_DURATION 3000  // 3 seconds hold required on GPIO 14 button

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
    lcd.setCursor(0, 0);
    lcd.print("LIFELINE RX  ");
    lcd.write(0); // Radar symbol
    lcd.print(" ");
    
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
        
        lcd.setCursor(14, 0);
        if (pulseState == 0) {
            lcd.write(0); // Radar glyph
        } else {
            lcd.print("o");
        }
        pulseState = (pulseState + 1) % 2;
        
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
    
    lcd.setCursor(0, 0);
    lcd.write(4); // Warning icon glyph
    
    char row0[17];
    snprintf(row0, sizeof(row0), "%c %-13s", code, nameShort);
    lcd.print(row0);
    
    const char* priLabel = (priority == 0) ? "CRIT" : (priority == 1) ? "HIGH" : (priority == 2) ? "MED " : "OK  ";
    
    char row1[17];
    snprintf(row1, sizeof(row1), "TX#%03d %d%s %s", deviceId % 1000, rssi, "dBm", priLabel);
    printLCDLine(1, row1);
    
    lastDeviceId = deviceId;
    lastAlertIndex = alertIndex;
    lastRssi = rssi;
    alertReceivedTime = millis();
    
    addToHistory(deviceId, alertIndex, rssi);
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
//            MULTI-WIFI, OPEN AP & WEB OTA FLASH MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════════════

void loadWiFiCredentials() {
    preferences.begin("lifeline", true);
    networkCount = 0;
    
    for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
        String keySSID = "ssid" + String(i + 1);
        String keyPass = "pass" + String(i + 1);
        String s = preferences.getString(keySSID.c_str(), "");
        String p = preferences.getString(keyPass.c_str(), "");
        
        if (s.length() > 0) {
            storedNetworks[networkCount].ssid = s;
            storedNetworks[networkCount].password = p;
            networkCount++;
        }
    }
    preferences.end();
    
    if (networkCount > 0) {
        activeSSID = storedNetworks[0].ssid;
        Serial.printf("[WIFI] Loaded %d WiFi network(s). Primary: %s\n", networkCount, activeSSID.c_str());
    } else {
        Serial.println(F("[WIFI] No stored WiFi networks found"));
    }
}

void saveWiFiCredentialsList(const WiFiNetwork nets[], int count) {
    preferences.begin("lifeline", false);
    for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
        String keySSID = "ssid" + String(i + 1);
        String keyPass = "pass" + String(i + 1);
        if (i < count && nets[i].ssid.length() > 0) {
            preferences.putString(keySSID.c_str(), nets[i].ssid);
            preferences.putString(keyPass.c_str(), nets[i].password);
        } else {
            preferences.remove(keySSID.c_str());
            preferences.remove(keyPass.c_str());
        }
    }
    preferences.end();
    
    loadWiFiCredentials();
}

void drawWiFiConnectingScreen(const String& ssid, int currentIdx, int totalCount) {
    printLCDLine(0, "WiFi Connecting");
    char line1[17];
    snprintf(line1, sizeof(line1), "%d/%d:%-11s", currentIdx, totalCount, ssid.c_str());
    printLCDLine(1, line1);
}

void drawWiFiConnectedScreen(const String& ip) {
    printLCDLine(0, "WiFi Connected!");
    String line1 = "IP:" + ip;
    printLCDLine(1, line1);
}

void drawWiFiFailedScreen() {
    printLCDLine(0, "WiFi Failed!");
    printLCDLine(1, "Open Setup AP..");
}

void drawCountdownScreen(int remainingSec) {
    printLCDLine(0, "GPIO14 Btn Held ");
    char line1[17];
    snprintf(line1, sizeof(line1), "Opening in %ds...", remainingSec);
    printLCDLine(1, line1);
}

bool connectToWiFiSilent() {
    if (networkCount == 0) {
        Serial.println(F("[WIFI] No networks configured for silent connect"));
        return false;
    }
    
    WiFi.mode(WIFI_STA);
    
    for (int i = 0; i < networkCount; i++) {
        Serial.printf("[WIFI] Silent connect attempt %d/%d to %s...\n", 
                      i + 1, networkCount, storedNetworks[i].ssid.c_str());
        WiFi.begin(storedNetworks[i].ssid.c_str(), storedNetworks[i].password.c_str());
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            activeSSID = storedNetworks[i].ssid;
            Serial.printf("[WIFI] Connected to %s! IP: %s\n", activeSSID.c_str(), WiFi.localIP().toString().c_str());
            return true;
        }
    }
    
    wifiConnected = false;
    Serial.println(F("[WIFI] All network connection attempts failed"));
    return false;
}

bool connectToWiFi() {
    if (networkCount == 0) {
        Serial.println(F("[WIFI] No credentials stored, launching open setup AP..."));
        drawWiFiFailedScreen();
        delay(1200);
        startWiFiPortal();
        return false;
    }
    
    WiFi.mode(WIFI_STA);
    
    for (int i = 0; i < networkCount; i++) {
        Serial.printf("[WIFI] Connecting to network %d/%d: %s...\n", 
                      i + 1, networkCount, storedNetworks[i].ssid.c_str());
        
        drawWiFiConnectingScreen(storedNetworks[i].ssid, i + 1, networkCount);
        WiFi.begin(storedNetworks[i].ssid.c_str(), storedNetworks[i].password.c_str());
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            activeSSID = storedNetworks[i].ssid;
            String ip = WiFi.localIP().toString();
            Serial.printf("[WIFI] Connected to %s! IP: %s\n", activeSSID.c_str(), ip.c_str());
            
            drawWiFiConnectedScreen(ip);
            delay(2000);
            return true;
        }
    }
    
    wifiConnected = false;
    Serial.println(F("[WIFI] All network connections failed - starting open setup portal"));
    
    drawWiFiFailedScreen();
    delay(1500);
    
    startWiFiPortal();
    return false;
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
    html += ".container{max-width:420px;margin:0 auto;background:#252542;padding:25px;border-radius:10px;}";
    html += "h1{color:#00d4ff;text-align:center;margin-bottom:5px;}";
    html += "p.subtitle{color:#00ff87;text-align:center;font-size:14px;margin-bottom:20px;}";
    html += "h2{color:#ffb800;font-size:15px;margin-top:15px;margin-bottom:8px;border-bottom:1px solid #444;padding-bottom:5px;}";
    html += "input[type=text],input[type=password]{width:100%;padding:10px;margin:4px 0 14px;border:1px solid #444;border-radius:5px;background:#1a1a2e;color:#fff;box-sizing:border-box;}";
    html += "input[type=submit]{width:100%;padding:14px;background:#00d4ff;color:#000;border:none;border-radius:5px;cursor:pointer;font-weight:bold;font-size:16px;margin-top:15px;}";
    html += "input[type=submit]:hover{background:#00b8e6;}";
    html += ".ota-btn{display:block;text-align:center;margin-top:18px;padding:12px;background:#2d2d44;color:#00ff87;border-radius:5px;text-decoration:none;font-weight:bold;}";
    html += ".status{text-align:center;margin-top:20px;padding:10px;border-radius:5px;font-size:13px;background:#2d2d44;color:#a3b1c6;}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>LifeLine RX</h1>";
    html += "<p class='subtitle'>Open WiFi Configuration Portal</p>";
    html += "<form action='/save' method='POST'>";
    
    for (int i = 0; i < 3; i++) {
        String numStr = String(i + 1);
        String currentS = (i < networkCount) ? storedNetworks[i].ssid : "";
        String currentP = (i < networkCount) ? storedNetworks[i].password : "";
        
        html += "<h2>WiFi Network #" + numStr + (i == 0 ? " (Primary)" : "") + "</h2>";
        html += "<label>SSID Name:</label>";
        html += "<input type='text' name='ssid" + numStr + "' placeholder='Network name' value='" + currentS + "'>";
        html += "<label>Password:</label>";
        html += "<input type='password' name='pass" + numStr + "' placeholder='Password' value='" + currentP + "'>";
    }
    
    html += "<input type='submit' value='Save All Networks'>";
    html += "</form>";
    
    html += "<a class='ota-btn' href='/update'>⚡ OTA Firmware Update Page</a>";
    
    if (networkCount > 0) {
        html += "<div class='status'>Configured Networks: " + String(networkCount) + " / 3</div>";
    }
    
    html += "</div></body></html>";
    return html;
}

String getUpdateHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LifeLine RX - OTA Update</title>";
    html += "<style>";
    html += "body{font-family:Arial;background:#1a1a2e;color:#fff;padding:25px;}";
    html += ".container{max-width:420px;margin:0 auto;background:#252542;padding:25px;border-radius:10px;}";
    html += "h1{color:#00d4ff;text-align:center;margin-bottom:10px;}";
    html += "p{color:#a3b1c6;text-align:center;font-size:14px;margin-bottom:20px;}";
    html += "input[type=file]{width:100%;padding:12px;margin:15px 0;background:#1a1a2e;color:#fff;border:1px solid #444;border-radius:5px;box-sizing:border-box;}";
    html += "input[type=submit]{width:100%;padding:14px;background:#00ff87;color:#000;border:none;border-radius:5px;font-weight:bold;font-size:16px;cursor:pointer;}";
    html += ".back{display:block;text-align:center;margin-top:20px;color:#00d4ff;text-decoration:none;font-weight:bold;}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>OTA Firmware Update</h1>";
    html += "<p>Upload compiled <b>firmware.bin</b> file to update LifeLine RX wirelessly over WiFi</p>";
    html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
    html += "<input type='file' name='update' accept='.bin' required>";
    html += "<input type='submit' value='Upload & Flash Firmware'>";
    html += "</form>";
    html += "<a class='back' href='/'>← Back to WiFi Setup Portal</a>";
    html += "</div></body></html>";
    return html;
}

void handlePortalRoot() {
    wifiServer.send(200, "text/html", getPortalHTML());
}

void handlePortalSave() {
    WiFiNetwork newNets[3];
    int count = 0;
    
    for (int i = 0; i < 3; i++) {
        String s = wifiServer.arg("ssid" + String(i + 1));
        String p = wifiServer.arg("pass" + String(i + 1));
        s.trim();
        p.trim();
        if (s.length() > 0) {
            newNets[count].ssid = s;
            newNets[count].password = p;
            count++;
        }
    }
    
    saveWiFiCredentialsList(newNets, count);
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LifeLine RX - Saved</title>";
    html += "<style>body{font-family:Arial;background:#1a1a2e;color:#fff;text-align:center;padding:50px;}";
    html += ".success{background:#00ff87;color:#000;padding:20px;border-radius:10px;max-width:400px;margin:0 auto;}</style></head><body>";
    html += "<div class='success'><h2>WiFi Networks Saved!</h2>";
    html += "<p>Configured: " + String(count) + " network(s)</p>";
    html += "<p>Device will restart and connect...</p></div>";
    html += "</body></html>";
    
    wifiServer.send(200, "text/html", html);
    
    delay(2000);
    ESP.restart();
}

void handleUpdateRoot() {
    wifiServer.send(200, "text/html", getUpdateHTML());
}

void handleUpdateDone() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LifeLine RX - Flashed</title>";
    html += "<style>body{font-family:Arial;background:#1a1a2e;color:#fff;text-align:center;padding:50px;}";
    html += ".success{background:#00ff87;color:#000;padding:20px;border-radius:10px;max-width:400px;margin:0 auto;}</style></head><body>";
    html += "<div class='success'><h2>OTA Update Successful!</h2>";
    html += "<p>Firmware flashed successfully. Device is restarting...</p></div>";
    html += "</body></html>";
    
    wifiServer.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
}

void handleUpdateUpload() {
    HTTPUpload& upload = wifiServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("[OTA] Update Start: %s\n", upload.filename.c_str());
        printLCDLine(0, "OTA Flashing...");
        printLCDLine(1, "Progress: 0%");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
        int percent = 0;
        if (Update.size() > 0) {
            percent = (Update.progress() * 100) / Update.size();
        }
        char prg[17];
        snprintf(prg, sizeof(prg), "Progress: %d%%", percent);
        printLCDLine(1, prg);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("[OTA] Success! Total size: %u B\n", upload.totalSize);
            printLCDLine(0, "OTA Success!");
            printLCDLine(1, "Rebooting...");
            playBootTone();
        } else {
            Update.printError(Serial);
            printLCDLine(0, "OTA Failed!");
            printLCDLine(1, "Error Flashing");
        }
    }
}

void startWiFiPortal() {
    Serial.println(F("[WIFI] Starting OPEN WiFi configuration portal..."));
    
    WiFi.disconnect(true);
    delay(100);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID);
    
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[WIFI] Open AP started! SSID: '%s' (Free / No Password)\n", WIFI_AP_SSID);
    Serial.printf("[WIFI] Portal IP: %s\n", apIP.toString().c_str());
    
    wifiServer.on("/", handlePortalRoot);
    wifiServer.on("/save", HTTP_POST, handlePortalSave);
    wifiServer.on("/update", HTTP_GET, handleUpdateRoot);
    wifiServer.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
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
    
    if (networkCount > 0) {
        connectToWiFi();
    }
    
    if (currentScreen != SCREEN_ALERT) {
        currentScreen = SCREEN_IDLE;
        drawIdleScreen();
    }
}

void drawWiFiPortalScreen() {
    printLCDLine(0, "AP:LifeLine-RX");
    printLCDLine(1, "Connect:192.168.4.1");
}

void checkWiFiPortalButton() {
    bool currentButtonState = digitalRead(WIFI_PORTAL_PIN);
    
    if (currentButtonState == LOW) { // Button on GPIO 14 active LOW
        if (!buttonPressed) {
            buttonPressed = true;
            buttonPressStartTime = millis();
            lastBeepSecond = -1;
        } else {
            unsigned long pressDuration = millis() - buttonPressStartTime;
            int secondsHeld = pressDuration / 1000;
            int remainingSec = 3 - secondsHeld;
            if (remainingSec < 1) remainingSec = 1;
            
            if (secondsHeld != lastBeepSecond && remainingSec >= 1 && remainingSec <= 3) {
                lastBeepSecond = secondsHeld;
                tone(BUZZER_PIN, 1800, 60);
            }
            
            if (!portalActive && pressDuration > 100) {
                drawCountdownScreen(remainingSec);
            }
            
            if (pressDuration >= LONG_PRESS_DURATION) {
                buttonPressed = false;
                tone(BUZZER_PIN, 2400, 200);
                
                if (portalActive) {
                    stopWiFiPortal();
                } else {
                    if (networkCount > 0) {
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
            lastBeepSecond = -1;
            
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
    Serial.println(F("║     16x2 LCD & Multi-WiFi (GPIO 14 Btn + OTA)             ║"));
    Serial.println(F("╚═══════════════════════════════════════════════════════════╝\n"));
    
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LORA_CS, OUTPUT);
    pinMode(LORA_RST, OUTPUT);
    pinMode(WIFI_PORTAL_PIN, INPUT_PULLUP); // Dedicated Push Button on GPIO 14
    
    digitalWrite(LORA_CS, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);
    
    Wire.begin(LCD_SDA, LCD_SCL);
    
    lcd.init();
    lcd.backlight();
    
    lcd.createChar(0, (uint8_t*)glyphRadar);
    lcd.createChar(1, (uint8_t*)glyphBell);
    lcd.createChar(2, (uint8_t*)glyphCheck);
    lcd.createChar(3, (uint8_t*)glyphSignal);
    lcd.createChar(4, (uint8_t*)glyphWarn);
    
    Serial.printf("[OK] 16x2 I2C LCD initialized (SDA: GPIO %d, SCL: GPIO %d, Addr: 0x%02X)\n", 
                  LCD_SDA, LCD_SCL, LCD_ADDR);
    
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    Serial.println(F("[OK] SPI initialized for LoRa"));
    
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
    
    currentScreen = SCREEN_BOOT;
    drawBootScreen();
    playBootTone();
    
    Serial.println(F("[OK] Boot screen displayed"));
    
    loadWiFiCredentials();
    if (networkCount > 0) {
        Serial.printf("[WIFI] Auto-connecting to %d stored network(s). Primary: %s\n", networkCount, activeSSID.c_str());
        connectToWiFiSilent();
    } else {
        Serial.println(F("[WIFI] No credentials - Hold GPIO 14 button for 3 seconds to configure"));
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
