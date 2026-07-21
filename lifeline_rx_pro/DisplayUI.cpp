#include "DisplayUI.h"
#include "BuzzerLED.h"
#include <Wire.h>

// Instantiate LCD
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// Define Global State Variables
ScreenState currentScreen = SCREEN_BOOT;
ScreenState previousScreenBeforePress = SCREEN_IDLE;
unsigned long bootStartTime = 0;
unsigned long noWiFiStartTime = 0;
unsigned long alertReceivedTime = 0;
unsigned long lastPulseTime = 0;
uint8_t pulseState = 0;
uint8_t bootDotState = 0;

int lastDeviceId = 0;
int lastAlertIndex = 0;
int lastRssi = 0;

AlertRecord alertHistory[HISTORY_MAX_ITEMS];
int historyCount = 0;
int historyScrollOffset = 0;
int totalAlertsReceived = 0;

void initDisplay() {
    Wire.begin(LCD_SDA, LCD_SCL);
    lcd.init();
    lcd.backlight();
    
    // Register custom glyphs (0 to 4)
    lcd.createChar(0, (uint8_t*)glyphRadar);  // CGRAM 0: Radar
    lcd.createChar(1, (uint8_t*)glyphBell);   // CGRAM 1: Bell
    lcd.createChar(2, (uint8_t*)glyphCheck);  // CGRAM 2: Checkmark
    lcd.createChar(3, (uint8_t*)glyphSignal); // CGRAM 3: Signal bars
    lcd.createChar(4, (uint8_t*)glyphWarn);   // CGRAM 4: Warning
    
    Serial.printf("[OK] 16x2 I2C LCD initialized (SDA: GPIO %d, SCL: GPIO %d, Addr: 0x%02X)\n", 
                  LCD_SDA, LCD_SCL, LCD_ADDR);
}

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

void drawBootScreen() {
    lcd.clear();
    printLCDCenter(0, "LIFELINE RX");
    printLCDCenter(1, "Booting...");
    bootStartTime = millis();
    bootDotState = 0;
    Serial.println(F("[SCREEN] Boot screen displayed on 16x2 LCD"));
}

bool updateBootAnimation() {
    unsigned long elapsed = millis() - bootStartTime;
    uint8_t newDotState = (elapsed / 300) % 4;
    
    if (newDotState != bootDotState) {
        bootDotState = newDotState;
        String status = "   Booting";
        for (int i = 0; i < bootDotState; i++) {
            status += ".";
        }
        printLCDLine(1, status);
    }
    
    return elapsed >= BOOT_DISPLAY_TIME;
}

#include <time.h>
#include <WiFi.h>

extern bool wifiConnected;

void initNTPTime() {
    if (wifiConnected || WiFi.status() == WL_CONNECTED) {
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
        Serial.println(F("[NTP] Initialized time synchronization"));
    }
}

bool getFormattedTimeStr(char* buffer, size_t maxLen) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) {
        return false;
    }
    snprintf(buffer, maxLen, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return true;
}

void drawNoWiFiScreen() {
    printLCDLine(0, " No Internet!   ");
    printLCDLine(1, "1x=Skip 3s=Setup");
    noWiFiStartTime = millis();
    Serial.println(F("[SCREEN] Displayed No Internet screen"));
}

void drawIdleScreen() {
    if (WiFi.status() == WL_CONNECTED || wifiConnected) {
        char timeBuf[12];
        if (getFormattedTimeStr(timeBuf, sizeof(timeBuf))) {
            char row0[17];
            snprintf(row0, sizeof(row0), "Time: %-10s", timeBuf);
            printLCDLine(0, row0);
        } else {
            printLCDLine(0, "Time: Syncing.. ");
        }
    } else {
        printLCDLine(0, "Offline Mode    ");
    }
    
    printLCDLine(1, "Waiting for TX..");
    
    lastPulseTime = millis();
    pulseState = 0;
    Serial.println(F("[SCREEN] Idle screen displayed on 16x2 LCD"));
}

void updateIdleAnimation() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastPulseTime >= 500) {
        lastPulseTime = currentTime;
        pulseState = (pulseState + 1) % 4;
        
        // Row 0: Time or Offline Mode
        if (WiFi.status() == WL_CONNECTED || wifiConnected) {
            char timeBuf[12];
            if (getFormattedTimeStr(timeBuf, sizeof(timeBuf))) {
                char row0[17];
                snprintf(row0, sizeof(row0), "Time: %-10s", timeBuf);
                printLCDLine(0, row0);
            } else {
                printLCDLine(0, "Time: Syncing.. ");
            }
        } else {
            printLCDLine(0, "Offline Mode    ");
        }
        
        // Row 1: Waiting for TX data animated dots
        switch (pulseState) {
            case 0: printLCDLine(1, "Waiting for TX  "); break;
            case 1: printLCDLine(1, "Waiting for TX. "); break;
            case 2: printLCDLine(1, "Waiting for TX.."); break;
            case 3: printLCDLine(1, "Waiting for TX..."); break;
        }
    }
}

void drawAlertScreen(int deviceId, int alertIndex, int rssi) {
    uint8_t priority = alertPriority[alertIndex];
    char code = getAlertCode(alertIndex);
    const char* nameShort = alertNamesShort[alertIndex];
    
    // Row 0: [!] A EMERGENCY
    lcd.setCursor(0, 0);
    lcd.write(4); // Warning icon glyph (CGRAM 4)
    
    char row0[17];
    snprintf(row0, sizeof(row0), " %c %-12s", code, nameShort);
    lcd.print(row0);
    
    // Row 1: TX#001 -65dBm CR
    const char* priCode = (priority < 5) ? priorityLabelsShort[priority] : "IN";
    
    char row1[17];
    snprintf(row1, sizeof(row1), "TX#%03d %4ddBm %s", deviceId % 1000, rssi, priCode);
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
    printLCDLine(0, "WiFi Failed!    ");
    printLCDLine(1, "Open Setup AP..");
}

void drawCountdownScreen(int remainingSec) {
    printLCDLine(0, "WiFi Btn Held   ");
    char line1[17];
    snprintf(line1, sizeof(line1), "Opening in %ds...", remainingSec);
    printLCDLine(1, line1);
}

void drawProgressCountdownScreen(unsigned long elapsedMs, unsigned long totalMs) {
    printLCDLine(0, "WiFi Btn Held   ");
    
    uint8_t filled = (elapsedMs * 16) / totalMs;
    if (filled > 16) filled = 16;
    
    char bar[17];
    for (uint8_t i = 0; i < 16; i++) {
        bar[i] = (i < filled) ? (char)0xFF : ' ';
    }
    bar[16] = '\0';
    
    lcd.setCursor(0, 1);
    lcd.print(bar);
}

void drawWiFiPortalScreen() {
    printLCDLine(0, "AP:LifeLine-RX  ");
    printLCDLine(1, "192.168.4.1     ");
}
