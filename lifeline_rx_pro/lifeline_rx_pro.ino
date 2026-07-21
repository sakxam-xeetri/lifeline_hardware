/*
 * ═══════════════════════════════════════════════════════════════════════════════════
 *                        ╔═════════════════════════════════════╗
 *                        ║    LIFELINE EMERGENCY RECEIVER      ║
 *                        ║   Professional Base Station v3.1    ║
 *                        ╚═════════════════════════════════════╝
 * ═══════════════════════════════════════════════════════════════════════════════════
 * 
 * Production-ready emergency communication receiver designed for deployment in 
 * remote Himalayan and rural regions of Nepal. Operated at base stations for
 * monitoring emergency alerts from field transmitters.
 * 
 * DESIGN PRINCIPLES:
 *   ✓ Clarity under stress - Large fonts, high contrast, clear icons
 *   ✓ High visibility alerts - Color-coded priority, glowing indicators
 *   ✓ Outdoor readability - Dark theme, no glare, industrial colors
 *   ✓ Rugged base station appearance - Government-grade emergency equipment look
 * 
 * HARDWARE SPECIFICATION:
 *   - Microcontroller: ESP32 Development Board
 *   - Display: 2.8" TFT SPI (Auto-adaptive: 320×240 or 320×480)
 *   - Communication: LoRa SX1278 @ 433 MHz
 *   - Indicators: Buzzer for alert notifications
 * 
 * PACKET FORMAT: TX[ID],[ALERT_CODE] (e.g., "TX003,A")
 * 
 * Author: LifeLine Development Team
 * Version: 3.1.0 PRO
 * License: MIT
 * ═══════════════════════════════════════════════════════════════════════════════════
 */

#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
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

// TFT Display Pins (ST7789) - Shares SPI bus with LoRa
// TFT uses same MOSI/MISO/SCK as LoRa, only CS is different
#define TFT_CS      16      // Chip Select (unique for TFT)
#define TFT_DC      4       // Data/Command
#define TFT_RST     2       // Reset
// TFT shares SPI bus: MOSI=GPIO27, MISO=GPIO17, SCK=GPIO5

// LED Indicator Pins (optional)
#define LED_GREEN   21      // Success indicator (active HIGH)
#define LED_RED     13      // Alert indicator (active HIGH)

// Audio Feedback
#define BUZZER_PIN  12      // Buzzer

// WiFi Portal Trigger Button
#define WIFI_PORTAL_PIN  0  // EN/BOOT button (GPIO 0) - Press to open WiFi portal

// ═══════════════════════════════════════════════════════════════════════════════════
//                          DISPLAY AUTO-ADAPTIVE CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════

// Physical display dimensions (will be auto-detected or set manually)
#define DISPLAY_240x320   1    // Standard 2.4" / 2.8" TFT
#define DISPLAY_320x480   2    // Larger 3.5" TFT

// Set your display type here (or implement auto-detection)
#define DISPLAY_TYPE  DISPLAY_240x320

#if DISPLAY_TYPE == DISPLAY_240x320
    #define NATIVE_WIDTH    240
    #define NATIVE_HEIGHT   320
#else
    #define NATIVE_WIDTH    320
    #define NATIVE_HEIGHT   480
#endif

// Landscape orientation (swaps width/height)
#define SCREEN_ROTATION     1   // 0=Portrait, 1=Landscape, 2=Portrait inverted, 3=Landscape inverted

// Computed screen dimensions after rotation
#if (SCREEN_ROTATION == 1 || SCREEN_ROTATION == 3)
    #define SCREEN_WIDTH    NATIVE_HEIGHT
    #define SCREEN_HEIGHT   NATIVE_WIDTH
#else
    #define SCREEN_WIDTH    NATIVE_WIDTH
    #define SCREEN_HEIGHT   NATIVE_HEIGHT
#endif

// ═══════════════════════════════════════════════════════════════════════════════════
//                              UI LAYOUT CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════════

// Fixed layout dimensions (consistent across all screens)
#define HEADER_HEIGHT       42                         // Fixed header height (taller for premium look)
#define FOOTER_HEIGHT       32                         // Fixed footer height
#define CONTENT_START_Y     (HEADER_HEIGHT + 4)        // Content area start (after accent line)
#define CONTENT_HEIGHT      (SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 6)
#define MARGIN              10                         // Universal margin (slightly larger)
#define MARGIN_SMALL        5                          // Small margin
#define BORDER_RADIUS       6                          // Rounded corner radius (more rounded)

// Text sizes (Adafruit GFX scale factors)
#define TEXT_XLARGE         4   // Extra large (boot logo) - 24px equiv
#define TEXT_LARGE          3   // Large headings - 18px equiv
#define TEXT_MEDIUM         2   // Standard text - 12px equiv
#define TEXT_SMALL          1   // Small text / labels - 6px equiv

// Premium UI Effects
#define GLOW_INTENSITY      3   // Glow effect layers
#define SHADOW_OFFSET       2   // Card shadow offset
#define GRADIENT_STEPS      8   // Steps for gradient simulation

// ═══════════════════════════════════════════════════════════════════════════════════
//                    PREMIUM PROFESSIONAL COLOR PALETTE
// ═══════════════════════════════════════════════════════════════════════════════════
// Ultra-modern dark theme with vibrant accents - inspired by high-end emergency systems
// ❌ NO WHITE OR LIGHT BACKGROUNDS - Premium dark aesthetic
// RGB888 to RGB565 conversion macro

#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// ─────────────────────────────── PRIMARY BACKGROUNDS ───────────────────────────────

// Ultra Deep Black - #0D0D0F (Primary background - Rich black)
#define COLOR_BG_PRIMARY        RGB565(13, 13, 15)

// Deep Navy Slate - #0a1929 (Headers/footers - Premium depth)
#define COLOR_BG_HEADER         RGB565(10, 25, 41)

// Midnight Blue - #071318 (Alternative header - Subtle variation)
#define COLOR_BG_HEADER_ALT     RGB565(7, 19, 24)

// Elevated Card - #1a1a2e (Cards with slight purple tint)
#define COLOR_BG_CARD           RGB565(26, 26, 46)

// Card Hover/Active - #252542
#define COLOR_BG_CARD_ACTIVE    RGB565(37, 37, 66)

// Input/Selection background - #2d2d44
#define COLOR_BG_INPUT          RGB565(45, 45, 68)

// Glass effect overlay simulation
#define COLOR_GLASS_LIGHT       RGB565(255, 255, 255)
#define COLOR_GLASS_DARK        RGB565(20, 20, 30)

// ─────────────────────────────── VIBRANT PRIMARY COLORS ────────────────────────────

// Electric Red - #ff3b3b (Critical/Emergency/Error - More vibrant)
#define COLOR_RED               RGB565(255, 59, 59)
#define COLOR_RED_DARK          RGB565(180, 30, 30)
#define COLOR_RED_BRIGHT        RGB565(255, 100, 100)
#define COLOR_RED_GLOW          RGB565(255, 80, 80)

// Neon Green - #00ff87 (Success - Electric feel)
#define COLOR_GREEN             RGB565(0, 255, 135)
#define COLOR_GREEN_DARK        RGB565(0, 180, 95)
#define COLOR_GREEN_BRIGHT      RGB565(50, 255, 160)
#define COLOR_GREEN_GLOW        RGB565(30, 255, 145)

// Electric Amber - #ffb800 (Selection/Warning - Rich gold)
#define COLOR_AMBER             RGB565(255, 184, 0)
#define COLOR_AMBER_DARK        RGB565(200, 140, 0)
#define COLOR_AMBER_BRIGHT      RGB565(255, 210, 50)
#define COLOR_AMBER_GLOW        RGB565(255, 195, 30)

// Cyan Electric - #00d4ff (Accent/Links - Vibrant cyan)
#define COLOR_CYAN              RGB565(0, 212, 255)
#define COLOR_CYAN_DARK         RGB565(0, 150, 200)
#define COLOR_CYAN_BRIGHT       RGB565(80, 230, 255)
#define COLOR_CYAN_GLOW         RGB565(40, 220, 255)

// Vivid Orange - #ff7b00 (High Priority - Punchy)
#define COLOR_ORANGE            RGB565(255, 123, 0)
#define COLOR_ORANGE_DARK       RGB565(200, 90, 0)
#define COLOR_ORANGE_BRIGHT     RGB565(255, 150, 50)

// Electric Purple - #a855f7 (Premium accent)
#define COLOR_PURPLE            RGB565(168, 85, 247)
#define COLOR_PURPLE_DARK       RGB565(120, 60, 180)

// ─────────────────────────────── TEXT COLORS ───────────────────────────────────────

// Primary Text - Pure White - #ffffff (Maximum contrast)
#define COLOR_TEXT_PRIMARY      RGB565(255, 255, 255)

// Secondary Text - Cool Gray - #a3b1c6
#define COLOR_TEXT_SECONDARY    RGB565(163, 177, 198)

// Muted Text - Blue Gray - #64748b
#define COLOR_TEXT_MUTED        RGB565(100, 116, 139)

// Dark Text (on light backgrounds) - #0a0a0a
#define COLOR_TEXT_DARK         RGB565(10, 10, 10)

// Disabled Text - Dark gray
#define COLOR_TEXT_DISABLED     RGB565(60, 70, 85)

// ─────────────────────────────── ACCENT & UTILITY ──────────────────────────────────

// Premium accent line color (subtle gradient effect)
#define COLOR_ACCENT_LINE       RGB565(40, 50, 70)
#define COLOR_ACCENT_BRIGHT     RGB565(60, 80, 120)

// Border colors - subtle but defined
#define COLOR_BORDER            RGB565(50, 60, 85)
#define COLOR_BORDER_FOCUS      COLOR_CYAN
#define COLOR_BORDER_SUCCESS    COLOR_GREEN
#define COLOR_BORDER_ERROR      COLOR_RED

// Status indicators with glow potential
#define COLOR_STATUS_CRITICAL   COLOR_RED
#define COLOR_STATUS_HIGH       COLOR_ORANGE
#define COLOR_STATUS_MEDIUM     COLOR_AMBER
#define COLOR_STATUS_LOW        COLOR_GREEN
#define COLOR_STATUS_NEUTRAL    COLOR_TEXT_SECONDARY

// Badge/Pill colors
#define COLOR_BADGE_BG          RGB565(45, 45, 75)

// Live indicator
#define COLOR_LIVE_DOT          COLOR_GREEN

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

// Shorter names for compact display
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

// Priority label text
const char* priorityLabels[] = {"CRITICAL", "HIGH", "MEDIUM", "OK", "INFO"};

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

// TFT Display (Hardware SPI - shared with LoRa module)
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ═══════════════════════════════════════════════════════════════════════════════════
//                              STATE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════════════

// Screen state enumeration
enum ScreenState {
    SCREEN_BOOT,            // 0 - Initial boot/splash screen
    SCREEN_IDLE,            // 1 - Waiting for signals
    SCREEN_ALERT,           // 2 - Alert display
    SCREEN_HISTORY,         // 3 - Alert history view
    SCREEN_SYSTEM_INFO      // 4 - System information display
};

// Current application state
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

// WiFi objects
WebServer wifiServer(80);
Preferences preferences;

// WiFi state variables
bool wifiConnected = false;
bool portalActive = false;
unsigned long portalStartTime = 0;
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
#define LONG_PRESS_DURATION 3000  // 3 seconds for long press

// Stored WiFi credentials
String storedSSID = "";
String storedPassword = "";

// ═══════════════════════════════════════════════════════════════════════════════════
//                          SERIAL DEBUG CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════

#define SERIAL_DEBUG_ENABLED true
#define SERIAL_BAUD_RATE     115200

String serialInputBuffer = "";

/**
 * Check for Serial input to simulate received alerts
 * Format: "DEVICE_ID,ALERT_CODE" (e.g., "3,A" or "3,5")
 */
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
    
    // Help command
    if (input == "h" || input == "H" || input == "help" || input == "?") {
        printSerialDebugMenu();
        return false;
    }
    
    // Quick single-digit command (1-9, 0)
    if (input.length() == 1 && ((input[0] >= '0' && input[0] <= '9'))) {
        deviceId = 1;
        alertIndex = (input[0] == '0') ? 9 : input[0] - '1';
        rssi = -65;
        Serial.printf("[SERIAL DEBUG] Quick alert: Device=%d, Alert=%d (%s)\n", 
                      deviceId, alertIndex, alertNames[alertIndex]);
        return true;
    }
    
    // Single letter alert code (A-O)
    if (input.length() == 1 && ((input[0] >= 'A' && input[0] <= 'O') || (input[0] >= 'a' && input[0] <= 'o'))) {
        deviceId = 1;
        char code = (input[0] >= 'a') ? (input[0] - 'a' + 'A') : input[0];
        alertIndex = code - 'A';
        rssi = -65;
        Serial.printf("[SERIAL DEBUG] Quick alert: Device=%d, Alert=%c (%s)\n", 
                      deviceId, code, alertNames[alertIndex]);
        return true;
    }
    
    // Parse full format: DEVICE_ID,ALERT_CODE
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

/**
 * Print debug menu for Serial Monitor
 */
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
//                              UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════════

/**
 * Get color based on alert priority level
 */
uint16_t getPriorityColor(uint8_t priority) {
    switch (priority) {
        case 0:  return COLOR_STATUS_CRITICAL;
        case 1:  return COLOR_STATUS_HIGH;
        case 2:  return COLOR_STATUS_MEDIUM;
        case 3:  return COLOR_STATUS_LOW;
        default: return COLOR_STATUS_NEUTRAL;
    }
}

/**
 * Get color for specific alert by index
 */
uint16_t getAlertColor(int index) {
    if (index < 0 || index >= ALERT_COUNT) return COLOR_STATUS_NEUTRAL;
    return getPriorityColor(alertPriority[index]);
}

/**
 * Get alert code character (A-O)
 */
char getAlertCode(int index) {
    if (index >= 0 && index < ALERT_COUNT) {
        return 'A' + index;
    }
    return 'X';
}

/**
 * Calculate RSSI bar segments (0-10)
 */
int getRssiBarLevel(int rssi) {
    if (rssi >= -50) return 10;
    if (rssi <= -110) return 1;
    return map(rssi, -110, -50, 1, 10);
}

/**
 * Draw text centered horizontally
 */
void drawCenteredText(const char* text, int y, uint8_t textSize, uint16_t color) {
    tft.setTextSize(textSize);
    tft.setTextColor(color);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, y);
    tft.print(text);
}

/**
 * Draw text right-aligned
 */
void drawRightText(const char* text, int y, uint8_t textSize, uint16_t color) {
    tft.setTextSize(textSize);
    tft.setTextColor(color);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(SCREEN_WIDTH - w - MARGIN, y);
    tft.print(text);
}

/**
 * Draw horizontal gradient effect (simulated with lines)
 */
void drawGradientH(int x, int y, int w, int h, uint16_t colorStart, uint16_t colorEnd) {
    for (int i = 0; i < w; i++) {
        uint8_t r1 = (colorStart >> 11) & 0x1F;
        uint8_t g1 = (colorStart >> 5) & 0x3F;
        uint8_t b1 = colorStart & 0x1F;
        uint8_t r2 = (colorEnd >> 11) & 0x1F;
        uint8_t g2 = (colorEnd >> 5) & 0x3F;
        uint8_t b2 = colorEnd & 0x1F;
        
        uint8_t r = r1 + (r2 - r1) * i / w;
        uint8_t g = g1 + (g2 - g1) * i / w;
        uint8_t b = b1 + (b2 - b1) * i / w;
        
        uint16_t color = (r << 11) | (g << 5) | b;
        tft.drawFastVLine(x + i, y, h, color);
    }
}

/**
 * Draw vertical gradient effect
 */
void drawGradientV(int x, int y, int w, int h, uint16_t colorTop, uint16_t colorBottom) {
    for (int i = 0; i < h; i++) {
        uint8_t r1 = (colorTop >> 11) & 0x1F;
        uint8_t g1 = (colorTop >> 5) & 0x3F;
        uint8_t b1 = colorTop & 0x1F;
        uint8_t r2 = (colorBottom >> 11) & 0x1F;
        uint8_t g2 = (colorBottom >> 5) & 0x3F;
        uint8_t b2 = colorBottom & 0x1F;
        
        uint8_t r = r1 + (r2 - r1) * i / h;
        uint8_t g = g1 + (g2 - g1) * i / h;
        uint8_t b = b1 + (b2 - b1) * i / h;
        
        uint16_t color = (r << 11) | (g << 5) | b;
        tft.drawFastHLine(x, y + i, w, color);
    }
}

/**
 * Draw a premium card with shadow effect
 */
void drawPremiumCard(int x, int y, int w, int h, uint16_t bgColor, uint16_t borderColor, bool hasGlow = false) {
    // Shadow (offset dark layer)
    tft.fillRoundRect(x + SHADOW_OFFSET, y + SHADOW_OFFSET, w, h, BORDER_RADIUS, RGB565(5, 5, 10));
    // Main card
    tft.fillRoundRect(x, y, w, h, BORDER_RADIUS, bgColor);
    // Border highlight on top-left (glass effect)
    tft.drawRoundRect(x, y, w, h, BORDER_RADIUS, borderColor);
    // Inner highlight line at top (subtle)
    tft.drawFastHLine(x + 4, y + 1, w - 8, RGB565(60, 60, 80));
}

/**
 * Draw a glowing icon circle
 */
void drawGlowCircle(int cx, int cy, int r, uint16_t color) {
    // Outer glow layers
    for (int i = GLOW_INTENSITY; i > 0; i--) {
        uint8_t cr = (color >> 11) & 0x1F;
        uint8_t cg = (color >> 5) & 0x3F;
        uint8_t cb = color & 0x1F;
        uint16_t glowColor = ((cr / (i + 1)) << 11) | ((cg / (i + 1)) << 5) | (cb / (i + 1));
        tft.drawCircle(cx, cy, r + i * 2, glowColor);
    }
    // Solid center
    tft.fillCircle(cx, cy, r, color);
}

/**
 * Draw a pulsing status indicator
 */
void drawStatusIndicator(int x, int y, int r, uint16_t color, bool active = true) {
    if (active) {
        tft.drawCircle(x, y, r + 3, RGB565((color >> 11) & 0x0F, (color >> 6) & 0x1F, (color >> 1) & 0x0F));
        tft.drawCircle(x, y, r + 2, RGB565((color >> 11) & 0x1F, (color >> 5) & 0x1F, (color) & 0x0F));
    }
    tft.fillCircle(x, y, r, color);
    tft.fillCircle(x - r/3, y - r/3, r/4, COLOR_TEXT_PRIMARY);
}

/**
 * Draw premium header bar with gradient and accent
 */
void drawHeader(const char* title) {
    // Gradient header background
    drawGradientV(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGB565(20, 40, 65), COLOR_BG_HEADER);
    
    // Glowing accent line below header
    tft.drawFastHLine(0, HEADER_HEIGHT - 2, SCREEN_WIDTH, COLOR_CYAN_DARK);
    tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_CYAN);
    tft.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_CYAN_DARK);
    
    // Title with slight shadow for depth
    tft.setTextSize(TEXT_MEDIUM);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
    int textY = (HEADER_HEIGHT - h) / 2 - 1;
    
    // Shadow
    tft.setTextColor(RGB565(0, 0, 0));
    tft.setCursor(MARGIN + 1, textY + 1);
    tft.print(title);
    
    // Main text
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN, textY);
    tft.print(title);
}

/**
 * Draw premium footer bar
 */
void drawFooter(const char* hints) {
    int footerY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    // Accent lines above footer
    tft.drawFastHLine(0, footerY - 2, SCREEN_WIDTH, COLOR_CYAN_DARK);
    tft.drawFastHLine(0, footerY - 1, SCREEN_WIDTH, COLOR_ACCENT_LINE);
    
    // Gradient footer background
    drawGradientV(0, footerY, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_BG_HEADER_ALT, RGB565(5, 12, 18));
    
    // Hint text
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(MARGIN, footerY + (FOOTER_HEIGHT - 8) / 2);
    tft.print(hints);
}

/**
 * Draw RSSI bar indicator (compact style for inline use)
 */
void drawRssiBar(int x, int y, int level, int rssi) {
    const int barWidth = 6;
    const int barGap = 2;
    const int maxHeight = 16;
    const int numBars = 6;
    
    // Draw compact bars
    for (int i = 0; i < numBars; i++) {
        int barHeight = 3 + (i * maxHeight / numBars);
        int barX = x + i * (barWidth + barGap);
        int barY = y + maxHeight - barHeight;
        
        if (i < (level * numBars / 10)) {
            uint16_t barColor;
            if (i < 2) barColor = COLOR_RED;
            else if (i < 4) barColor = COLOR_AMBER;
            else barColor = COLOR_GREEN;
            tft.fillRect(barX, barY, barWidth, barHeight, barColor);
        } else {
            tft.drawRect(barX, barY, barWidth, barHeight, COLOR_BORDER);
        }
    }
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

/**
 * Add alert to history
 */
void addToHistory(int deviceId, int alertIndex, int rssi) {
    // Shift history if full
    if (historyCount >= HISTORY_MAX_ITEMS) {
        for (int i = 0; i < HISTORY_MAX_ITEMS - 1; i++) {
            alertHistory[i] = alertHistory[i + 1];
        }
        historyCount = HISTORY_MAX_ITEMS - 1;
    }
    
    // Add new alert
    alertHistory[historyCount].deviceId = deviceId;
    alertHistory[historyCount].alertIndex = alertIndex;
    alertHistory[historyCount].rssi = rssi;
    alertHistory[historyCount].timestamp = millis();
    historyCount++;
    totalAlertsReceived++;
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              1. BOOT SCREEN
//         Purpose: System identity, trust, and readiness indicator
//         Design: Authoritative, centered, industrial appearance
// ═══════════════════════════════════════════════════════════════════════════════════

void drawBootScreen() {
    // Dark background
    tft.fillScreen(COLOR_BG_PRIMARY);
    
    // ─────────────────── TOP ACCENT LINE ───────────────────
    tft.drawFastHLine(0, 0, SCREEN_WIDTH, COLOR_CYAN_DARK);
    tft.drawFastHLine(0, 1, SCREEN_WIDTH, COLOR_CYAN);
    tft.drawFastHLine(0, 2, SCREEN_WIDTH, COLOR_CYAN_DARK);
    
    // ─────────────────── MEDICAL CROSS ICON ───────────────────
    int iconX = SCREEN_WIDTH / 2;
    int iconY = 45;
    int crossW = 8;
    int crossH = 24;
    
    // Cross with glow effect
    tft.fillRect(iconX - crossW/2 - 1, iconY - crossH/2 - 1, crossW + 2, crossH + 2, COLOR_RED_DARK);
    tft.fillRect(iconX - crossH/2 - 1, iconY - crossW/2 - 1, crossH + 2, crossW + 2, COLOR_RED_DARK);
    tft.fillRect(iconX - crossW/2, iconY - crossH/2, crossW, crossH, COLOR_RED);
    tft.fillRect(iconX - crossH/2, iconY - crossW/2, crossH, crossW, COLOR_RED);
    
    // ─────────────────── MAIN TITLE ───────────────────
    tft.setTextSize(TEXT_LARGE);
    int16_t x1, y1;
    uint16_t tw, th;
    tft.getTextBounds("LifeLine", 0, 0, &x1, &y1, &tw, &th);
    int titleX = (SCREEN_WIDTH - tw) / 2;
    int titleY = 78;
    
    // Shadow
    tft.setTextColor(RGB565(0, 50, 70));
    tft.setCursor(titleX + 1, titleY + 1);
    tft.print(F("LifeLine"));
    
    // Main text
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(titleX, titleY);
    tft.print(F("LifeLine"));
    
    // ─────────────────── SUBTITLE ───────────────────
    drawCenteredText("EMERGENCY RECEIVER", 108, TEXT_SMALL, COLOR_CYAN);
    
    // Separator line
    int lineY = 122;
    tft.drawFastHLine(50, lineY, SCREEN_WIDTH - 100, COLOR_ACCENT_LINE);
    tft.fillCircle(50, lineY, 2, COLOR_CYAN_DARK);
    tft.fillCircle(SCREEN_WIDTH - 50, lineY, 2, COLOR_CYAN_DARK);
    
    // ─────────────────── DEVICE TYPE BADGE ───────────────────
    int badgeW = 100;
    int badgeX = (SCREEN_WIDTH - badgeW) / 2;
    int badgeY = 132;
    
    tft.fillRoundRect(badgeX, badgeY, badgeW, 22, 4, COLOR_BG_CARD);
    tft.drawRoundRect(badgeX, badgeY, badgeW, 22, 4, COLOR_CYAN_DARK);
    drawCenteredText("BASE STATION", badgeY + 7, TEXT_SMALL, COLOR_CYAN);
    
    // ─────────────────── INFO CARDS ───────────────────
    int cardY = SCREEN_HEIGHT - 55;
    int cardW = (SCREEN_WIDTH - MARGIN * 3) / 2;
    int cardH = 40;
    
    // Left card - LoRa Status
    tft.fillRoundRect(MARGIN, cardY, cardW, cardH, 4, COLOR_BG_CARD);
    tft.drawRoundRect(MARGIN, cardY, cardW, cardH, 4, COLOR_BORDER);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 8, cardY + 6);
    tft.print(F("LORA"));
    
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(MARGIN + 8, cardY + 20);
    tft.print(F("433 MHz"));
    
    // Right card - Status
    int rightCardX = MARGIN * 2 + cardW;
    tft.fillRoundRect(rightCardX, cardY, cardW, cardH, 4, COLOR_BG_CARD);
    tft.drawRoundRect(rightCardX, cardY, cardW, cardH, 4, COLOR_BORDER);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(rightCardX + 8, cardY + 6);
    tft.print(F("STATUS"));
    
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(rightCardX + 8, cardY + 20);
    tft.print(F("Ready"));
    
    // ─────────────────── BOTTOM ACCENT ───────────────────
    tft.drawFastHLine(0, SCREEN_HEIGHT - 3, SCREEN_WIDTH, COLOR_CYAN_DARK);
    tft.drawFastHLine(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH, COLOR_CYAN);
    tft.drawFastHLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, COLOR_CYAN_DARK);
    
    bootStartTime = millis();
    bootDotState = 0;
    Serial.println(F("[SCREEN] Boot screen displayed"));
}

/**
 * Update boot screen loading animation
 * Returns true when boot is complete
 */
bool updateBootAnimation() {
    unsigned long elapsed = millis() - bootStartTime;
    
    uint8_t newDotState = (elapsed / 400) % 4;
    
    if (newDotState != bootDotState) {
        bootDotState = newDotState;
        
        // Update loading indicator in right card
        int cardY = SCREEN_HEIGHT - 55;
        int cardW = (SCREEN_WIDTH - MARGIN * 3) / 2;
        int rightCardX = MARGIN * 2 + cardW;
        
        // Clear and redraw status text
        tft.fillRect(rightCardX + 8, cardY + 20, 65, 10, COLOR_BG_CARD);
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_GREEN);
        tft.setCursor(rightCardX + 8, cardY + 20);
        tft.print(F("Ready"));
        for (int i = 0; i < bootDotState; i++) {
            tft.print(F("."));
        }
    }
    
    return elapsed >= BOOT_DISPLAY_TIME;
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              2. IDLE SCREEN
//         Purpose: Calm waiting state with live status indicators
//         Design: Clean, minimal, with animated listening indicator
// ═══════════════════════════════════════════════════════════════════════════════════

void drawIdleScreen() {
    // Premium dark background
    tft.fillScreen(COLOR_BG_PRIMARY);
    
    // ─────────────────── PREMIUM HEADER ───────────────────
    drawHeader("LIFELINE RX");
    
    // Live indicator badge in header (right side)
    int badgeX = SCREEN_WIDTH - 52;
    tft.fillRoundRect(badgeX, 13, 42, 14, 3, RGB565(10, 35, 20));
    tft.drawRoundRect(badgeX, 13, 42, 14, 3, COLOR_GREEN_DARK);
    tft.fillCircle(badgeX + 9, 20, 3, COLOR_GREEN);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(badgeX + 16, 16);
    tft.print(F("LIVE"));
    
    // ─────────────────── MAIN STATUS CARD ───────────────────
    int mainCardY = HEADER_HEIGHT + 12;
    int mainCardH = 85;
    
    // Main listening card background
    tft.fillRoundRect(MARGIN, mainCardY, SCREEN_WIDTH - MARGIN * 2, mainCardH, 6, COLOR_BG_CARD);
    tft.drawRoundRect(MARGIN, mainCardY, SCREEN_WIDTH - MARGIN * 2, mainCardH, 6, COLOR_BORDER);
    
    // Inner content area
    int contentCenterX = SCREEN_WIDTH / 2;
    int contentCenterY = mainCardY + 35;
    
    // Radar/Signal icon (simple and clean)
    // Outer ring
    tft.drawCircle(contentCenterX, contentCenterY, 22, COLOR_CYAN_DARK);
    tft.drawCircle(contentCenterX, contentCenterY, 21, COLOR_CYAN_DARK);
    // Middle ring
    tft.drawCircle(contentCenterX, contentCenterY, 14, COLOR_CYAN);
    // Inner filled circle
    tft.fillCircle(contentCenterX, contentCenterY, 6, COLOR_CYAN);
    tft.fillCircle(contentCenterX, contentCenterY, 3, COLOR_CYAN_BRIGHT);
    
    // Radar sweep line (static)
    tft.drawLine(contentCenterX, contentCenterY, contentCenterX + 18, contentCenterY - 12, COLOR_CYAN_BRIGHT);
    
    // Status text below icon
    drawCenteredText("LISTENING", mainCardY + 68, TEXT_SMALL, COLOR_TEXT_PRIMARY);
    
    // ─────────────────── STATUS MESSAGE ───────────────────
    int msgY = mainCardY + mainCardH + 10;
    drawCenteredText("Waiting for emergency signals...", msgY, TEXT_SMALL, COLOR_TEXT_MUTED);
    
    // ─────────────────── PULSE INDICATORS ───────────────────
    int pulseY = msgY + 18;
    for (int i = 0; i < 3; i++) {
        int x = SCREEN_WIDTH / 2 - 22 + (i * 22);
        tft.drawCircle(x, pulseY, 4, COLOR_BORDER);
    }
    
    // ─────────────────── STATS ROW ───────────────────
    int statsY = SCREEN_HEIGHT - FOOTER_HEIGHT - 38;
    int statW = (SCREEN_WIDTH - MARGIN * 4) / 3;
    
    // Stat 1: LoRa Frequency
    tft.fillRoundRect(MARGIN, statsY, statW, 28, 4, COLOR_BG_CARD);
    tft.drawRoundRect(MARGIN, statsY, statW, 28, 4, COLOR_BORDER);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 6, statsY + 4);
    tft.print(F("FREQ"));
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(MARGIN + 6, statsY + 15);
    tft.print(F("433MHz"));
    
    // Stat 2: Alerts count
    int stat2X = MARGIN * 2 + statW;
    tft.fillRoundRect(stat2X, statsY, statW, 28, 4, COLOR_BG_CARD);
    tft.drawRoundRect(stat2X, statsY, statW, 28, 4, COLOR_BORDER);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(stat2X + 6, statsY + 4);
    tft.print(F("ALERTS"));
    tft.setTextColor(COLOR_AMBER);
    tft.setCursor(stat2X + 6, statsY + 15);
    tft.print(totalAlertsReceived);
    
    // Stat 3: Status
    int stat3X = MARGIN * 3 + statW * 2;
    tft.fillRoundRect(stat3X, statsY, statW, 28, 4, COLOR_BG_CARD);
    tft.drawRoundRect(stat3X, statsY, statW, 28, 4, COLOR_BORDER);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(stat3X + 6, statsY + 4);
    tft.print(F("STATUS"));
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(stat3X + 6, statsY + 15);
    tft.print(F("READY"));
    
    // ─────────────────── PREMIUM FOOTER ───────────────────
    drawFooter("Auto-receiving mode");
    
    lastPulseTime = millis();
    pulseState = 0;
    Serial.println(F("[SCREEN] Idle screen displayed"));
}

/**
 * Update idle screen animations (non-blocking)
 */
void updateIdleAnimation() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastPulseTime >= IDLE_PULSE_INTERVAL) {
        lastPulseTime = currentTime;
        
        // Calculate positions (must match drawIdleScreen)
        int mainCardY = HEADER_HEIGHT + 12;
        int mainCardH = 85;
        int msgY = mainCardY + mainCardH + 10;
        int pulseY = msgY + 18;
        
        // Clear and redraw pulse indicators
        for (int i = 0; i < 3; i++) {
            int x = SCREEN_WIDTH / 2 - 22 + (i * 22);
            tft.fillCircle(x, pulseY, 6, COLOR_BG_PRIMARY);
            tft.drawCircle(x, pulseY, 4, COLOR_BORDER);
        }
        
        // Draw active pulse
        int activeX = SCREEN_WIDTH / 2 - 22 + (pulseState * 22);
        tft.fillCircle(activeX, pulseY, 4, COLOR_CYAN);
        
        pulseState = (pulseState + 1) % 3;
        
        // Update live indicator pulse in header
        int badgeX = SCREEN_WIDTH - 52;
        bool bright = (pulseState == 0);
        tft.fillCircle(badgeX + 9, 20, 3, bright ? COLOR_GREEN_BRIGHT : COLOR_GREEN);
        
        // Animate radar sweep (optional subtle effect)
        int contentCenterX = SCREEN_WIDTH / 2;
        int contentCenterY = mainCardY + 35;
        
        // Clear previous sweep
        tft.drawLine(contentCenterX, contentCenterY, 
                     contentCenterX + 18, contentCenterY - 12, COLOR_BG_CARD);
        
        // Draw new sweep based on state
        int sweepAngles[3][2] = {{18, -12}, {20, 0}, {14, 14}};
        tft.drawLine(contentCenterX, contentCenterY,
                     contentCenterX + sweepAngles[pulseState][0], 
                     contentCenterY + sweepAngles[pulseState][1], COLOR_CYAN_BRIGHT);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              3. ALERT SCREEN
//         Purpose: Maximum visibility for incoming emergency alerts
//         Design: Color-coded priority, large text, glowing indicators
// ═══════════════════════════════════════════════════════════════════════════════════

void drawAlertScreen(int deviceId, int alertIndex, int rssi) {
    uint16_t alertColor = getAlertColor(alertIndex);
    uint8_t priority = alertPriority[alertIndex];
    
    // Dark background
    tft.fillScreen(COLOR_BG_PRIMARY);
    
    // ─────────────────── ALERT HEADER ───────────────────
    tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, alertColor);
    tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_TEXT_PRIMARY);
    
    // Warning triangle icon
    int triX = MARGIN + 12;
    int triY = HEADER_HEIGHT / 2;
    tft.fillTriangle(triX, triY - 8, triX - 8, triY + 6, triX + 8, triY + 6, COLOR_TEXT_DARK);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(alertColor);
    tft.setCursor(triX - 3, triY - 2);
    tft.print('!');
    
    // Header title
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(MARGIN + 28, (HEADER_HEIGHT - 14) / 2);
    tft.print(F("INCOMING ALERT"));
    
    // Active badge (right side)
    int activeBadgeX = SCREEN_WIDTH - 58;
    tft.fillRoundRect(activeBadgeX, 12, 48, 16, 3, COLOR_TEXT_DARK);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(alertColor);
    tft.setCursor(activeBadgeX + 5, 15);
    tft.print(F("ACTIVE"));
    
    // ─────────────────── PRIORITY BADGE ───────────────────
    int badgeY = HEADER_HEIGHT + 8;
    int badgeW = 65;
    tft.fillRoundRect(MARGIN, badgeY, badgeW, 16, 3, alertColor);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(MARGIN + 5, badgeY + 4);
    tft.print(priorityLabels[min((int)priority, 4)]);
    
    // ─────────────────── MAIN ALERT CARD ───────────────────
    int cardY = badgeY + 24;
    int cardH = 42;
    int cardW = SCREEN_WIDTH - MARGIN * 2;
    
    // Card background
    tft.fillRoundRect(MARGIN, cardY, cardW, cardH, 4, COLOR_BG_CARD);
    tft.drawRoundRect(MARGIN, cardY, cardW, cardH, 4, alertColor);
    
    // Left accent bar
    tft.fillRect(MARGIN, cardY, 4, cardH, alertColor);
    
    // Alert name
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(alertColor);
    tft.setCursor(MARGIN + 12, cardY + 8);
    tft.print(alertNamesShort[alertIndex]);
    
    // Alert code badge (right side of card)
    int codeBadgeX = SCREEN_WIDTH - MARGIN - 32;
    tft.fillRoundRect(codeBadgeX, cardY + 6, 24, 16, 3, alertColor);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(codeBadgeX + 6, cardY + 8);
    tft.print(getAlertCode(alertIndex));
    
    // Alert index text
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 12, cardY + 28);
    tft.print(F("Alert #"));
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.print(alertIndex + 1);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.print(F(" of "));
    tft.print(ALERT_COUNT);
    
    // ─────────────────── DEVICE & SIGNAL ROW ───────────────────
    int infoY = cardY + cardH + 10;
    int halfW = (SCREEN_WIDTH - MARGIN * 3) / 2;
    
    // Device ID card (left)
    tft.fillRoundRect(MARGIN, infoY, halfW, 34, 4, COLOR_BG_CARD);
    tft.drawRoundRect(MARGIN, infoY, halfW, 34, 4, COLOR_CYAN_DARK);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 6, infoY + 4);
    tft.print(F("FROM DEVICE"));
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(MARGIN + 6, infoY + 17);
    tft.print(F("TX #"));
    if (deviceId < 100) tft.print('0');
    if (deviceId < 10) tft.print('0');
    tft.print(deviceId);
    
    // Signal card (right)
    int rssiCardX = MARGIN * 2 + halfW;
    tft.fillRoundRect(rssiCardX, infoY, halfW, 34, 4, COLOR_BG_CARD);
    tft.drawRoundRect(rssiCardX, infoY, halfW, 34, 4, COLOR_BORDER);
    
    // Signal label and value
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(rssiCardX + 6, infoY + 4);
    tft.print(F("SIGNAL"));
    
    // RSSI value
    char rssiStr[10];
    sprintf(rssiStr, "%d dBm", rssi);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(rssiCardX + 50, infoY + 4);
    tft.print(rssiStr);
    
    // Mini RSSI bars
    int rssiLevel = getRssiBarLevel(rssi);
    int barStartX = rssiCardX + 6;
    int barY = infoY + 20;
    for (int i = 0; i < 5; i++) {
        int barH = 4 + i * 2;
        int barX = barStartX + i * 10;
        uint16_t barColor = (i < rssiLevel / 2) ? 
            ((i < 2) ? COLOR_RED : ((i < 3) ? COLOR_AMBER : COLOR_GREEN)) : COLOR_BORDER;
        tft.fillRect(barX, barY + 12 - barH, 7, barH, barColor);
    }
    
    // ─────────────────── VERIFIED BADGE ───────────────────
    int statusY = infoY + 44;
    int statusW = 90;
    int statusX = (SCREEN_WIDTH - statusW) / 2;
    
    tft.fillRoundRect(statusX, statusY, statusW, 22, 4, COLOR_GREEN);
    
    // Simple checkmark
    int checkX = statusX + 12;
    int checkY = statusY + 11;
    tft.drawLine(checkX - 4, checkY, checkX - 1, checkY + 4, COLOR_TEXT_DARK);
    tft.drawLine(checkX - 1, checkY + 4, checkX + 6, checkY - 3, COLOR_TEXT_DARK);
    tft.drawLine(checkX - 4, checkY + 1, checkX - 1, checkY + 5, COLOR_TEXT_DARK);
    tft.drawLine(checkX - 1, checkY + 5, checkX + 6, checkY - 2, COLOR_TEXT_DARK);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(statusX + 26, statusY + 7);
    tft.print(F("VERIFIED"));
    
    // ─────────────────── FOOTER ───────────────────
    drawFooter("Auto-dismiss in 30 seconds");
    
    // Store alert data
    lastDeviceId = deviceId;
    lastAlertIndex = alertIndex;
    lastRssi = rssi;
    alertReceivedTime = millis();
    
    // Add to history
    addToHistory(deviceId, alertIndex, rssi);
    
    // Play alert sound
    playAlertTone(priority);
    
    Serial.printf("[SCREEN] Alert displayed: Device %d, Alert %d (%s)\n", 
                  deviceId, alertIndex, alertNames[alertIndex]);
}

/**
 * Check if alert display time has elapsed
 */
bool shouldReturnToIdle() {
    if (currentScreen != SCREEN_ALERT) return false;
    return (millis() - alertReceivedTime >= ALERT_DISPLAY_TIME);
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              LORA PACKET HANDLING
// ═══════════════════════════════════════════════════════════════════════════════════

/**
 * Parse incoming LoRa packet
 * Expected format: DEVICE_ID,ALERT_CODE or TX[ID],[CODE]
 */
bool parseLoRaPacket(int& deviceId, int& alertIndex, int& rssi) {
    int packetSize = LoRa.parsePacket();
    if (packetSize == 0) return false;
    
    String data = "";
    while (LoRa.available()) {
        data += (char)LoRa.read();
    }
    
    rssi = LoRa.packetRssi();
    
    Serial.printf("[RX] Raw packet (%d bytes): '%s', RSSI: %d\n", packetSize, data.c_str(), rssi);
    
    // Handle TX prefix if present
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
    
    // Handle character-based alert code (A-O = 0-14)
    if (alertPart.length() == 1 && alertPart[0] >= 'A' && alertPart[0] <= 'O') {
        alertIndex = alertPart[0] - 'A';
    } else if (alertPart.length() == 1 && alertPart[0] >= 'a' && alertPart[0] <= 'o') {
        alertIndex = alertPart[0] - 'a';
    } else {
        alertIndex = alertPart.toInt();
    }
    
    // Validate
    if (alertIndex < 0 || alertIndex >= ALERT_COUNT) {
        Serial.printf("[RX] Invalid alert index %d, defaulting to OTHER\n", alertIndex);
        alertIndex = ALERT_COUNT - 1;
    }
    
    Serial.printf("[RX] Parsed: Device=%d, Alert=%d (%s)\n", deviceId, alertIndex, alertNames[alertIndex]);
    
    // Re-enter receive mode for next packet
    LoRa.receive();
    
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              WIFI & API FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════════════

/**
 * Load WiFi credentials from persistent storage
 */
void loadWiFiCredentials() {
    preferences.begin("lifeline", true);  // Read-only
    storedSSID = preferences.getString("ssid", "");
    storedPassword = preferences.getString("password", "");
    preferences.end();
    
    if (storedSSID.length() > 0) {
        Serial.printf("[WIFI] Loaded credentials for SSID: %s\n", storedSSID.c_str());
    } else {
        Serial.println(F("[WIFI] No stored credentials found"));
    }
}

/**
 * Save WiFi credentials to persistent storage
 */
void saveWiFiCredentials(const String& ssid, const String& password) {
    preferences.begin("lifeline", false);  // Read-write
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    
    storedSSID = ssid;
    storedPassword = password;
    Serial.printf("[WIFI] Saved credentials for SSID: %s\n", ssid.c_str());
}

/**
 * Draw WiFi connecting screen on TFT
 */
void drawWiFiConnectingScreen(const String& ssid) {
    tft.fillScreen(COLOR_BG_PRIMARY);
    
    // Header
    tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG_HEADER);
    tft.setTextColor(COLOR_CYAN);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(MARGIN, 12);
    tft.print("WiFi Connecting");
    
    // Content
    int y = CONTENT_START_Y + 30;
    
    tft.setTextColor(COLOR_AMBER);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(MARGIN, y);
    tft.print("Connecting to:");
    
    y += 30;
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN, y);
    tft.print(ssid);
    
    y += 40;
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(MARGIN, y);
    tft.print("Please wait...");
}

/**
 * Draw WiFi connected screen with IP
 */
void drawWiFiConnectedScreen(const String& ip) {
    tft.fillScreen(COLOR_BG_PRIMARY);
    
    // Header
    tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG_HEADER);
    tft.setTextColor(COLOR_GREEN);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(MARGIN, 12);
    tft.print("WiFi Connected!");
    
    // Content
    int y = CONTENT_START_Y + 30;
    
    tft.setTextColor(COLOR_AMBER);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(MARGIN, y);
    tft.print("Network:");
    
    y += 28;
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN, y);
    tft.print(storedSSID);
    
    y += 40;
    tft.setTextColor(COLOR_AMBER);
    tft.setCursor(MARGIN, y);
    tft.print("IP Address:");
    
    y += 28;
    tft.setTextColor(COLOR_GREEN);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(MARGIN, y);
    tft.print(ip);
    
    // Footer
    tft.fillRect(0, SCREEN_HEIGHT - FOOTER_HEIGHT, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_BG_HEADER);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setTextSize(TEXT_SMALL);
    tft.setCursor(MARGIN, SCREEN_HEIGHT - FOOTER_HEIGHT + 12);
    tft.print("Returning to main screen...");
}

/**
 * Draw WiFi failed screen
 */
void drawWiFiFailedScreen() {
    tft.fillScreen(COLOR_BG_PRIMARY);
    
    // Header
    tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG_HEADER);
    tft.setTextColor(COLOR_RED);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(MARGIN, 12);
    tft.print("WiFi Failed!");
    
    // Content
    int y = CONTENT_START_Y + 30;
    
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(MARGIN, y);
    tft.print("Connection failed");
    
    y += 35;
    tft.setTextColor(COLOR_AMBER);
    tft.setCursor(MARGIN, y);
    tft.print("Opening setup");
    
    y += 28;
    tft.setCursor(MARGIN, y);
    tft.print("portal...");
}

/**
 * Connect to WiFi silently at boot (no display, no auto-portal on failure)
 * Used for automatic connection on startup
 */
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

/**
 * Connect to WiFi using stored credentials with display feedback
 */
bool connectToWiFi() {
    if (storedSSID.length() == 0) {
        Serial.println(F("[WIFI] No credentials stored"));
        return false;
    }
    
    Serial.printf("[WIFI] Connecting to %s...\n", storedSSID.c_str());
    
    // Show connecting screen
    drawWiFiConnectingScreen(storedSSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(storedSSID.c_str(), storedPassword.c_str());
    
    unsigned long startTime = millis();
    int dotCount = 0;
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.print(".");
        
        // Update dots on screen
        tft.setTextColor(COLOR_CYAN);
        tft.setTextSize(TEXT_MEDIUM);
        tft.setCursor(MARGIN + (dotCount * 12), CONTENT_START_Y + 100);
        tft.print(".");
        dotCount = (dotCount + 1) % 10;
        if (dotCount == 0) {
            tft.fillRect(MARGIN, CONTENT_START_Y + 95, 150, 25, COLOR_BG_PRIMARY);
        }
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        String ip = WiFi.localIP().toString();
        Serial.printf("[WIFI] Connected! IP: %s\n", ip.c_str());
        
        // Show connected screen
        drawWiFiConnectedScreen(ip);
        delay(2000);  // Show for 2 seconds
        
        return true;
    } else {
        wifiConnected = false;
        Serial.println(F("[WIFI] Connection failed"));
        
        // Show failed screen
        drawWiFiFailedScreen();
        delay(1500);
        
        // Auto-open portal on failure
        Serial.println(F("[WIFI] Auto-opening configuration portal..."));
        startWiFiPortal();
        
        return false;
    }
}

/**
 * Push alert data to web dashboard API
 */
void pushAlertToAPI(int deviceId, int alertIndex, int rssi) {
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[API] WiFi not connected, skipping API push"));
        return;
    }
    
    HTTPClient http;
    http.begin(API_ENDPOINT);
    http.addHeader("Content-Type", "application/json");
    
    // Create JSON payload: { DID: device_id, message_code: alert_code, RSSI: rssi }
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

/**
 * Generate WiFi portal HTML page
 */
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

/**
 * Handle root page request
 */
void handlePortalRoot() {
    wifiServer.send(200, "text/html", getPortalHTML());
}

/**
 * Handle save credentials request
 */
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

/**
 * Start WiFi configuration portal (AP mode)
 */
void startWiFiPortal() {
    Serial.println(F("[WIFI] Starting configuration portal..."));
    
    // Stop any existing connection
    WiFi.disconnect(true);
    delay(100);
    
    // Start AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[WIFI] Portal started! Connect to '%s' (password: %s)\n", WIFI_AP_SSID, WIFI_AP_PASSWORD);
    Serial.printf("[WIFI] Portal IP: %s\n", apIP.toString().c_str());
    
    // Setup web server routes
    wifiServer.on("/", handlePortalRoot);
    wifiServer.on("/save", HTTP_POST, handlePortalSave);
    wifiServer.begin();
    
    portalActive = true;
    portalStartTime = millis();
    
    // Show portal info on TFT
    drawWiFiPortalScreen();
}

/**
 * Stop WiFi configuration portal
 */
void stopWiFiPortal() {
    if (!portalActive) return;
    
    Serial.println(F("[WIFI] Stopping portal..."));
    wifiServer.stop();
    WiFi.softAPdisconnect(true);
    portalActive = false;
    
    // Try to connect with stored credentials
    if (storedSSID.length() > 0) {
        connectToWiFi();
    }
    
    // Return to idle screen
    if (currentScreen != SCREEN_ALERT) {
        currentScreen = SCREEN_IDLE;
        drawIdleScreen();
    }
}

/**
 * Draw WiFi portal screen on TFT
 */
void drawWiFiPortalScreen() {
    tft.fillScreen(COLOR_BG_PRIMARY);
    
    // Header
    tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG_HEADER);
    tft.setTextColor(COLOR_CYAN);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(MARGIN, 12);
    tft.print("WiFi Setup");
    
    // Content
    int y = CONTENT_START_Y + 10;
    
    tft.setTextColor(COLOR_AMBER);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(MARGIN, y);
    tft.print("Connect to WiFi:");
    
    y += 30;
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN, y);
    tft.print(WIFI_AP_SSID);
    
    y += 30;
    tft.setTextColor(COLOR_AMBER);
    tft.setCursor(MARGIN, y);
    tft.print("Password:");
    
    y += 25;
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN, y);
    tft.print(WIFI_AP_PASSWORD);
    
    y += 35;
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(MARGIN, y);
    tft.print("Then open:");
    
    y += 25;
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(MARGIN, y);
    tft.print("192.168.4.1");
    
    // Footer
    tft.fillRect(0, SCREEN_HEIGHT - FOOTER_HEIGHT, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_BG_HEADER);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setTextSize(TEXT_SMALL);
    tft.setCursor(MARGIN, SCREEN_HEIGHT - FOOTER_HEIGHT + 12);
    tft.print("Hold EN 3s to exit");
}

/**
 * Draw long press progress on screen
 */
void drawLongPressProgress(float progress) {
    int barWidth = SCREEN_WIDTH - (2 * MARGIN);
    int barHeight = 8;
    int barY = SCREEN_HEIGHT - FOOTER_HEIGHT - 15;
    
    // Background bar
    tft.fillRect(MARGIN, barY, barWidth, barHeight, COLOR_BG_CARD);
    
    // Progress bar
    int progressWidth = (int)(barWidth * progress);
    if (progressWidth > 0) {
        tft.fillRect(MARGIN, barY, progressWidth, barHeight, COLOR_CYAN);
    }
    
    // Text
    tft.fillRect(MARGIN, barY - 20, barWidth, 18, COLOR_BG_PRIMARY);
    tft.setTextColor(COLOR_AMBER);
    tft.setTextSize(TEXT_SMALL);
    tft.setCursor(MARGIN, barY - 15);
    if (progress < 1.0) {
        tft.print("Hold EN button for WiFi setup...");
    } else {
        tft.print("Activating WiFi setup...");
    }
}

/**
 * Check WiFi portal button (EN/GPIO0) - 3 second long press
 */
void checkWiFiPortalButton() {
    bool currentButtonState = digitalRead(WIFI_PORTAL_PIN);
    
    // Button pressed (LOW = pressed on EN button)
    if (currentButtonState == LOW) {
        if (!buttonPressed) {
            // Button just pressed - start timing
            buttonPressed = true;
            buttonPressStartTime = millis();
        } else {
            // Button held - check duration
            unsigned long pressDuration = millis() - buttonPressStartTime;
            float progress = (float)pressDuration / LONG_PRESS_DURATION;
            
            if (progress > 1.0) progress = 1.0;
            
            // Show progress on current screen (only if not in portal mode)
            if (!portalActive && pressDuration > 200) {  // Start showing after 200ms
                drawLongPressProgress(progress);
            }
            
            // Long press detected
            if (pressDuration >= LONG_PRESS_DURATION) {
                buttonPressed = false;  // Reset to prevent re-trigger
                
                if (portalActive) {
                    // Exit portal mode
                    stopWiFiPortal();
                } else {
                    // Enter WiFi setup - try connecting first, portal opens on failure
                    if (storedSSID.length() > 0) {
                        connectToWiFi();  // This will auto-open portal if it fails
                    } else {
                        startWiFiPortal();  // No credentials, open portal directly
                    }
                }
                
                // Redraw current screen to remove progress bar
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
        // Button released
        if (buttonPressed) {
            buttonPressed = false;
            
            // Redraw screen to remove progress bar (if short press)
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

/**
 * Handle WiFi portal in main loop
 */
void handleWiFiPortal() {
    if (!portalActive) return;
    
    wifiServer.handleClient();
    
    // Auto-timeout portal
    if (millis() - portalStartTime >= WIFI_PORTAL_TIMEOUT) {
        Serial.println(F("[WIFI] Portal timeout, closing..."));
        stopWiFiPortal();
    }
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              SETUP
// ═══════════════════════════════════════════════════════════════════════════════════

void setup() {
    // Initialize Serial for debugging
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
    
    Serial.println(F("\n╔═══════════════════════════════════════════════════════════╗"));
    Serial.println(F("║      LIFELINE EMERGENCY RECEIVER v3.1 PRO                 ║"));
    Serial.println(F("║            Professional Base Station                      ║"));
    Serial.println(F("╚═══════════════════════════════════════════════════════════╝\n"));
    
    // Initialize pins
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(TFT_CS, OUTPUT);
    pinMode(LORA_CS, OUTPUT);
    pinMode(LORA_RST, OUTPUT);
    pinMode(WIFI_PORTAL_PIN, INPUT_PULLUP);  // WiFi portal button (EN/GPIO0)
    
    digitalWrite(TFT_CS, HIGH);
    digitalWrite(LORA_CS, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    
    // Manual LoRa reset for reliable initialization
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);
    
    // Initialize SPI
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    Serial.println(F("[OK] SPI initialized"));
    
    // Initialize LoRa - WORKING CONFIGURATION
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
    
    // Initialize TFT Display
    tft.init(NATIVE_WIDTH, NATIVE_HEIGHT);
    tft.setRotation(SCREEN_ROTATION);
    tft.fillScreen(ST77XX_BLACK);
    
    Serial.printf("[OK] TFT initialized (%dx%d)\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Show boot screen
    currentScreen = SCREEN_BOOT;
    drawBootScreen();
    playBootTone();
    
    Serial.println(F("[OK] Boot screen displayed"));
    
    // Load and auto-connect WiFi if credentials exist
    loadWiFiCredentials();
    if (storedSSID.length() > 0) {
        Serial.printf("[WIFI] Auto-connecting to: %s\n", storedSSID.c_str());
        // Try to connect (but don't open portal on failure during boot)
        connectToWiFiSilent();
    } else {
        Serial.println(F("[WIFI] No credentials - Hold EN button for 3 seconds to configure"));
    }
    
    Serial.println(F("=== Ready to receive emergency alerts ===\n"));
    
    // Print Serial debug menu
    #if SERIAL_DEBUG_ENABLED
    printSerialDebugMenu();
    #endif
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              MAIN LOOP
// ═══════════════════════════════════════════════════════════════════════════════════

void loop() {
    // Handle WiFi portal if active
    if (portalActive) {
        checkWiFiPortalButton();  // Allow exiting portal
        handleWiFiPortal();
        return;  // Don't process other things while portal is active
    }
    
    // State machine for screen management
    switch (currentScreen) {
        
        case SCREEN_BOOT:
            // Update boot animation and check if complete
            if (updateBootAnimation()) {
                currentScreen = SCREEN_IDLE;
                drawIdleScreen();
                digitalWrite(LED_GREEN, HIGH);
                Serial.println(F("[STATE] Switched to IDLE - Listening for alerts"));
            }
            break;
            
        case SCREEN_IDLE:
            // Check WiFi button (3 second long press)
            checkWiFiPortalButton();
            
            // Update idle animation
            updateIdleAnimation();
            
            // Check for incoming LoRa packets (priority)
            {
                int deviceId, alertIndex, rssi;
                bool packetReceived = parseLoRaPacket(deviceId, alertIndex, rssi);
                
                // If no LoRa packet, check for Serial simulated packet
                #if SERIAL_DEBUG_ENABLED
                if (!packetReceived) {
                    packetReceived = checkSerialSimulatedPacket(deviceId, alertIndex, rssi);
                }
                #endif
                
                if (packetReceived) {
                    Serial.printf("[RX] Alert received: Device=%d, Alert=%d (%s), RSSI=%d\n",
                                  deviceId, alertIndex, alertNames[alertIndex], rssi);
                    
                    // Push alert to web dashboard API
                    pushAlertToAPI(deviceId, alertIndex, rssi);
                    
                    currentScreen = SCREEN_ALERT;
                    drawAlertScreen(deviceId, alertIndex, rssi);
                    
                    // Set LED based on priority
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
            // Check WiFi button (3 second long press)
            checkWiFiPortalButton();
            
            // Check for new packets (higher priority alert would override)
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
                    
                    // Push alert to web dashboard API
                    pushAlertToAPI(deviceId, alertIndex, rssi);
                    
                    drawAlertScreen(deviceId, alertIndex, rssi);
                    
                    if (alertPriority[alertIndex] <= 1) {
                        digitalWrite(LED_RED, HIGH);
                        digitalWrite(LED_GREEN, LOW);
                    }
                }
            }
            
            // Return to idle after display time
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
            // Reserved for future expansion
            break;
    }
    
    // Small delay to prevent CPU hogging
    delay(10);
}
