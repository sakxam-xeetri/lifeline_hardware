/*
 * ═══════════════════════════════════════════════════════════════════════════════════
 *                        ╔═════════════════════════════════════╗
 *                        ║   LIFELINE EMERGENCY TRANSMITTER    ║
 *                        ║   Professional Field Unit v3.1      ║
 *                        ╚═════════════════════════════════════╝
 * ═══════════════════════════════════════════════════════════════════════════════════
 * 
 * Production-ready emergency communication transmitter designed for deployment in 
 * remote Himalayan and rural regions of Nepal. Operated by villagers, mountaineers, 
 * and rescue coordinators during critical emergency situations.
 * 
 * DESIGN PRINCIPLES:
 *   ✓ Clarity under stress - Large fonts, high contrast, clear icons
 *   ✓ High reliability - Fail-safe operation, confirmation dialogs
 *   ✓ Minimal interaction - Direct key selection, intuitive navigation
 *   ✓ Outdoor readability - Dark theme, no glare, industrial colors
 *   ✓ Rugged field device appearance - Government-grade emergency equipment look
 * 
 * HARDWARE SPECIFICATION:
 *   - Microcontroller: ESP32 Development Board
 *   - Display: 2.8" TFT SPI (Auto-adaptive: 320×240 or 320×480)
 *   - Communication: LoRa SX1278 @ 433 MHz
 *   - Input: 4×4 Matrix Keypad (Primary) / Serial Monitor (Debug)
 *   - Indicators: Green LED (Success), Red LED (Failure), Buzzer
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
#include <Keypad.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ═══════════════════════════════════════════════════════════════════════════════════
//                              DEVICE CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════

#define DEVICE_ID           3              // Unique transmitter ID (1-999)
#define DEVICE_NAME         "LifeLine TX"  // Device display name
#define FIRMWARE_VERSION    "v3.1.0 PRO"   // Firmware version string
#define DEVICE_TYPE         "TX"           // Device type identifier
#define LORA_FREQUENCY      433E6          // LoRa frequency in Hz (433 MHz)
#define LORA_SF             12             // LoRa Spreading Factor (7-12) - Max range
#define LORA_BW             125E3          // LoRa Bandwidth (125 kHz) - WORKING CONFIGURATION
#define LORA_TX_POWER       18             // LoRa TX Power in dBm (2-20)
#define LORA_CR             8              // LoRa Coding Rate 4/8 - Max error correction
#define LORA_PREAMBLE       16             // Preamble Length - Good for weak signals
#define LORA_SYNC_WORD      0x12           // LoRa Sync Word (default, must match RX)

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

// LED Indicator Pins
#define LED_GREEN   21      // Success indicator (active HIGH)
#define LED_RED     13      // Failure indicator (active HIGH) - Changed from 22

// Audio Feedback
#define BUZZER_PIN  12      // Buzzer

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

// Menu layout
#define MENU_ITEM_HEIGHT    34                         // Height per menu item (more spacious)
#define VISIBLE_MENU_ITEMS  5                          // Visible items in scroll list
#define MENU_START_Y        (HEADER_HEIGHT + 28)       // Menu list start position

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

// ═══════════════════════════════════════════════════════════════════════════════════
//                              KEYPAD CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════

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

// Create keypad instance
Keypad keypad = Keypad(makeKeymap(keypadLayout), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

// ═══════════════════════════════════════════════════════════════════════════════════
//                              ALERT DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════════════

#define ALERT_COUNT 15

// Alert messages - MUST match receiver configuration
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

// Alert codes for LoRa transmission (A-O)
char getAlertCode(int index) {
    if (index >= 0 && index < ALERT_COUNT) {
        return 'A' + index;
    }
    return 'X';  // Invalid
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              TIMING CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════════

#define BOOT_DISPLAY_TIME       1000    // Boot screen duration (ms) - 1 second
#define SENDING_MIN_DISPLAY     400     // Minimum sending screen time (ms) - FAST
#define RESULT_SUCCESS_TIME     1800    // Success screen auto-return (ms) - FAST
#define RESULT_FAILURE_TIME     0       // Failure requires user input (no auto-return)
#define DEBOUNCE_DELAY          30      // Keypress debounce (ms) - FAST
#define SCROLL_REPEAT_DELAY     80      // Auto-scroll repeat delay (ms) - FAST
#define MAX_RETRY_ATTEMPTS      3       // Maximum transmission retry attempts

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
    SCREEN_MENU,            // 1 - Main alert selection menu
    SCREEN_CONFIRM,         // 2 - Alert confirmation dialog
    SCREEN_SENDING,         // 3 - Transmission in progress
    SCREEN_RESULT,          // 4 - Transmission result (success/failure)
    SCREEN_SYSTEM_INFO,     // 5 - System information display
    SCREEN_USER_MANUAL      // 6 - User manual / help screen
};

// Current application state
ScreenState currentScreen = SCREEN_BOOT;
ScreenState previousScreen = SCREEN_BOOT;

// Menu navigation state
int selectedAlertIndex = 0;     // Currently highlighted alert (0 to ALERT_COUNT-1)
int menuScrollOffset = 0;       // First visible item in scroll list

// Manual screen state  
int manualPage = 0;             // Current page in manual (0-indexed)
#define MANUAL_TOTAL_PAGES 4    // Total pages in manual

// Timing state
unsigned long bootStartTime = 0;
unsigned long resultStartTime = 0;
unsigned long lastKeyPressTime = 0;
unsigned long lastTransmitTime = 0;

// Transmission state
bool lastTransmitSuccess = false;
int retryCount = 0;
int totalTransmissions = 0;
int successfulTransmissions = 0;

// System status
bool loraInitialized = false;
int batteryPercent = -1;  // -1 = not available

// ═══════════════════════════════════════════════════════════════════════════════════
//                          SERIAL DEBUG CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════

#define SERIAL_DEBUG_ENABLED true
#define SERIAL_BAUD_RATE     115200

/**
 * Read key from Serial Monitor (Debug/Fallback mode)
 */
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

/**
 * Print Serial debug header
 */
void printDebugHeader() {
    Serial.println(F("\n╔═══════════════════════════════════════════════════════════╗"));
    Serial.println(F("║       LIFELINE EMERGENCY TRANSMITTER v3.1 PRO             ║"));
    Serial.println(F("║              Serial Debug Mode Active                     ║"));
    Serial.println(F("╚═══════════════════════════════════════════════════════════╝\n"));
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
 * Update scroll position to keep selection visible
 */
void updateMenuScroll() {
    if (selectedAlertIndex < menuScrollOffset) {
        menuScrollOffset = selectedAlertIndex;
    } else if (selectedAlertIndex >= menuScrollOffset + VISIBLE_MENU_ITEMS) {
        menuScrollOffset = selectedAlertIndex - VISIBLE_MENU_ITEMS + 1;
    }
    menuScrollOffset = constrain(menuScrollOffset, 0, max(0, ALERT_COUNT - VISIBLE_MENU_ITEMS));
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
        // Glow effect
        tft.drawCircle(x, y, r + 3, RGB565((color >> 11) & 0x0F, (color >> 6) & 0x1F, (color >> 1) & 0x0F));
        tft.drawCircle(x, y, r + 2, RGB565((color >> 11) & 0x1F, (color >> 5) & 0x1F, (color) & 0x0F));
    }
    tft.fillCircle(x, y, r, color);
    // Highlight dot
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
 * Draw a key hint badge (e.g., [*] SELECT)
 */
void drawKeyBadge(int x, int y, char key, const char* label, uint16_t keyColor) {
    // Key badge background
    tft.fillRoundRect(x, y - 2, 14, 12, 2, COLOR_BADGE_BG);
    tft.drawRoundRect(x, y - 2, 14, 12, 2, keyColor);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(keyColor);
    tft.setCursor(x + 4, y);
    tft.print(key);
    
    // Label
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(x + 18, y);
    tft.print(label);
}

/**
 * Draw a separator line
 */
void drawSeparator(int y) {
    tft.drawFastHLine(MARGIN, y, SCREEN_WIDTH - (2 * MARGIN), COLOR_ACCENT_LINE);
}

/**
 * Set LED state with feedback
 */
void setLED(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

void clearAllLEDs() {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
}

/**
 * Audio feedback functions - NON-BLOCKING for instant UI response
 */
void playSuccessTone() {
    tone(BUZZER_PIN, 2200, 80);  // Short, non-blocking
}

void playErrorTone() {
    tone(BUZZER_PIN, 800, 150);  // Single beep, no loop delays
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

// ═══════════════════════════════════════════════════════════════════════════════════
//                              1. BOOT SCREEN
//         Purpose: System identity, trust, and readiness indicator
//         Design: Authoritative, centered, industrial appearance
// ═══════════════════════════════════════════════════════════════════════════════════

void drawBootScreen() {
    // Initial screen - premium dark gradient background
    drawGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGB565(15, 20, 35), COLOR_BG_PRIMARY);
    
    // ─────────────────── DECORATIVE TOP ACCENT BAR ───────────────────
    drawGradientH(0, 0, SCREEN_WIDTH, 4, COLOR_CYAN_DARK, COLOR_PURPLE_DARK);
    
    // ─────────────────── GLOWING MEDICAL CROSS ICON ───────────────────
    int crossCenterX = SCREEN_WIDTH / 2;
    int crossCenterY = 50;
    int crossSize = 22;
    int crossThick = 10;
    
    // Outer glow effect
    for (int g = 4; g > 0; g--) {
        uint16_t glowColor = RGB565(80 - g*15, 20 - g*4, 20 - g*4);
        tft.fillRect(crossCenterX - crossThick/2 - g, crossCenterY - crossSize - g, 
                     crossThick + g*2, crossSize * 2 + g*2, glowColor);
        tft.fillRect(crossCenterX - crossSize - g, crossCenterY - crossThick/2 - g, 
                     crossSize * 2 + g*2, crossThick + g*2, glowColor);
    }
    
    // Main red cross
    tft.fillRect(crossCenterX - crossThick/2, crossCenterY - crossSize, 
                 crossThick, crossSize * 2, COLOR_RED);
    tft.fillRect(crossCenterX - crossSize, crossCenterY - crossThick/2, 
                 crossSize * 2, crossThick, COLOR_RED);
    
    // Highlight on cross (3D effect)
    tft.fillRect(crossCenterX - crossThick/2 + 2, crossCenterY - crossSize + 2, 
                 2, crossSize - 4, COLOR_RED_BRIGHT);
    
    // ─────────────────── MAIN TITLE "LifeLine" WITH GLOW ───────────────────
    tft.setTextSize(TEXT_XLARGE);
    int16_t x1, y1;
    uint16_t tw, th;
    tft.getTextBounds("LifeLine", 0, 0, &x1, &y1, &tw, &th);
    int titleX = (SCREEN_WIDTH - tw) / 2;
    int titleY = 85;
    
    // Text glow/shadow
    tft.setTextColor(COLOR_CYAN_DARK);
    tft.setCursor(titleX + 2, titleY + 2);
    tft.print(F("LifeLine"));
    
    // Main title
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(titleX, titleY);
    tft.print(F("LifeLine"));
    
    // Animated underline effect (gradient)
    for (int i = 0; i < tw; i++) {
        uint16_t lineColor = (i < tw/2) ? COLOR_CYAN : COLOR_PURPLE;
        tft.drawPixel(titleX + i, titleY + th + 6, lineColor);
        tft.drawPixel(titleX + i, titleY + th + 7, lineColor);
    }
    
    // ─────────────────── SUBTITLE WITH PREMIUM STYLING ───────────────────
    drawCenteredText("EMERGENCY COMMUNICATION", 130, TEXT_SMALL, COLOR_CYAN);
    
    // Decorative dots
    tft.fillCircle(SCREEN_WIDTH/2 - 60, 144, 2, COLOR_ACCENT_LINE);
    drawCenteredText("TRANSMITTER", 140, TEXT_MEDIUM, COLOR_TEXT_SECONDARY);
    tft.fillCircle(SCREEN_WIDTH/2 + 60, 144, 2, COLOR_ACCENT_LINE);
    
    // ─────────────────── PREMIUM INFO CARDS ───────────────────
    int cardY = SCREEN_HEIGHT - 65;
    int cardW = (SCREEN_WIDTH - MARGIN * 3) / 2;
    
    // Left card - Device ID
    drawPremiumCard(MARGIN, cardY, cardW, 50, COLOR_BG_CARD, COLOR_BORDER);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(MARGIN + 8, cardY + 8);
    tft.print(F("DEVICE"));
    
    char deviceIdStr[16];
    sprintf(deviceIdStr, "TX #%03d", DEVICE_ID);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN + 8, cardY + 26);
    tft.print(deviceIdStr);
    
    // Right card - Status
    int rightCardX = MARGIN * 2 + cardW;
    drawPremiumCard(rightCardX, cardY, cardW, 50, COLOR_BG_CARD, COLOR_BORDER);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(rightCardX + 8, cardY + 8);
    tft.print(F("STATUS"));
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(rightCardX + 8, cardY + 24);
    tft.print(F("Initializing..."));
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(rightCardX + 8, cardY + 36);
    tft.print(FIRMWARE_VERSION);
    
    // ─────────────────── BOTTOM DECORATIVE LINE ───────────────────
    drawGradientH(0, SCREEN_HEIGHT - 3, SCREEN_WIDTH, 3, COLOR_PURPLE_DARK, COLOR_CYAN_DARK);
    
    bootStartTime = millis();
    Serial.println(F("[SCREEN] Premium boot screen displayed"));
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                         2. MAIN ALERT SELECTION MENU
//    Purpose: Fast, error-free emergency alert selection
//    Design: Clear list, yellow highlight, scroll indicator, industrial header/footer
// ═══════════════════════════════════════════════════════════════════════════════════

/**
 * FAST PARTIAL UPDATE: Update only the changed menu items (instant response)
 */
void updateMenuSelection(int oldIndex, int newIndex) {
    int listStartY = HEADER_HEIGHT + 26;
    int itemHeight = MENU_ITEM_HEIGHT;
    
    // Redraw old selection (if visible) - Premium deselected style
    if (oldIndex >= menuScrollOffset && oldIndex < menuScrollOffset + VISIBLE_MENU_ITEMS) {
        int i = oldIndex - menuScrollOffset;
        int itemY = listStartY + (i * itemHeight);
        int itemWidth = SCREEN_WIDTH - (2 * MARGIN);
        
        // Clear with background
        tft.fillRect(MARGIN, itemY, itemWidth, itemHeight - 2, COLOR_BG_PRIMARY);
        
        // Subtle border for definition
        tft.drawRoundRect(MARGIN, itemY, itemWidth, itemHeight - 3, 4, COLOR_BORDER);
        
        // Number badge
        tft.fillRoundRect(MARGIN + 4, itemY + 6, 22, 18, 3, COLOR_BG_CARD);
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(MARGIN + 8, itemY + 9);
        if (oldIndex + 1 < 10) tft.print(' ');
        tft.print(oldIndex + 1);
        
        // Alert name
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(MARGIN + 32, itemY + 9);
        tft.print(alertNamesShort[oldIndex]);
        
        // Priority indicator with glow
        uint16_t prioColor = getAlertColor(oldIndex);
        tft.fillCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 15, 5, prioColor);
    }
    
    // Redraw new selection - Premium selected style with glow effect
    if (newIndex >= menuScrollOffset && newIndex < menuScrollOffset + VISIBLE_MENU_ITEMS) {
        int i = newIndex - menuScrollOffset;
        int itemY = listStartY + (i * itemHeight);
        int itemWidth = SCREEN_WIDTH - (2 * MARGIN);
        
        // Glow/shadow effect
        tft.fillRoundRect(MARGIN + 2, itemY + 2, itemWidth, itemHeight - 3, 5, RGB565(60, 55, 0));
        
        // Main amber selection background
        tft.fillRoundRect(MARGIN, itemY, itemWidth, itemHeight - 3, 5, COLOR_AMBER);
        
        // Highlight bar on left edge
        tft.fillRect(MARGIN, itemY, 4, itemHeight - 3, COLOR_AMBER_BRIGHT);
        
        // Inner highlight (glass effect)
        tft.drawFastHLine(MARGIN + 6, itemY + 2, itemWidth - 20, COLOR_AMBER_BRIGHT);
        
        // Number badge (inverted)
        tft.fillRoundRect(MARGIN + 6, itemY + 5, 24, 20, 3, COLOR_TEXT_DARK);
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_AMBER);
        tft.setCursor(MARGIN + 10, itemY + 8);
        if (newIndex + 1 < 10) tft.print(' ');
        tft.print(newIndex + 1);
        
        // Alert name (dark on amber)
        tft.setTextColor(COLOR_TEXT_DARK);
        tft.setCursor(MARGIN + 36, itemY + 8);
        tft.print(alertNamesShort[newIndex]);
        
        // Priority indicator with border
        uint16_t prioColor = getAlertColor(newIndex);
        if (prioColor == COLOR_AMBER) prioColor = COLOR_ORANGE_DARK;
        tft.fillCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 6, prioColor);
        tft.drawCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 7, COLOR_TEXT_DARK);
        tft.drawCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 8, COLOR_TEXT_DARK);
    }
}

void drawMenuScreen() {
    // INSTANT TRANSITION - Draw header first, then fill content area
    drawHeader("SELECT ALERT TYPE");
    
    // Fill content area below header - instant overwrite (no full screen clear)
    tft.fillRect(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 2, COLOR_BG_PRIMARY);
    
    // Device ID badge in header
    char idStr[10];
    sprintf(idStr, "#%03d", DEVICE_ID);
    tft.fillRoundRect(SCREEN_WIDTH - 48, 8, 42, 18, 4, COLOR_BG_CARD);
    tft.drawRoundRect(SCREEN_WIDTH - 48, 8, 42, 18, 4, COLOR_CYAN_DARK);
    drawRightText(idStr, 12, TEXT_SMALL, COLOR_CYAN);
    
    // ─────────────────── SCROLL INDICATOR BAR (Premium) ───────────────────
    int indicatorY = HEADER_HEIGHT + 8;
    
    // Progress bar background
    int barWidth = 80;
    int barX = SCREEN_WIDTH - MARGIN - barWidth;
    tft.fillRoundRect(barX, indicatorY, barWidth, 10, 3, COLOR_BG_CARD);
    
    // Progress fill
    int fillWidth = (menuScrollOffset + VISIBLE_MENU_ITEMS) * barWidth / ALERT_COUNT;
    int startFill = menuScrollOffset * barWidth / ALERT_COUNT;
    tft.fillRoundRect(barX + startFill, indicatorY + 1, fillWidth - startFill, 8, 2, COLOR_CYAN_DARK);
    
    // Range text
    int firstItem = menuScrollOffset + 1;
    int lastItem = min(menuScrollOffset + VISIBLE_MENU_ITEMS, ALERT_COUNT);
    char rangeStr[16];
    sprintf(rangeStr, "%d-%d of %d", firstItem, lastItem, ALERT_COUNT);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN, indicatorY + 1);
    tft.print(rangeStr);
    
    // ─────────────────── PREMIUM ALERT LIST ───────────────────
    int listStartY = HEADER_HEIGHT + 26;
    int itemHeight = MENU_ITEM_HEIGHT;
    
    for (int i = 0; i < VISIBLE_MENU_ITEMS; i++) {
        int alertIndex = menuScrollOffset + i;
        if (alertIndex >= ALERT_COUNT) break;
        
        int itemY = listStartY + (i * itemHeight);
        int itemWidth = SCREEN_WIDTH - (2 * MARGIN);
        bool isSelected = (alertIndex == selectedAlertIndex);
        
        if (isSelected) {
            // ─── SELECTED ITEM: Premium amber with glow ───
            // Glow/shadow
            tft.fillRoundRect(MARGIN + 2, itemY + 2, itemWidth, itemHeight - 3, 5, RGB565(60, 55, 0));
            
            // Main background
            tft.fillRoundRect(MARGIN, itemY, itemWidth, itemHeight - 3, 5, COLOR_AMBER);
            
            // Left accent bar
            tft.fillRect(MARGIN, itemY, 4, itemHeight - 3, COLOR_AMBER_BRIGHT);
            
            // Glass highlight
            tft.drawFastHLine(MARGIN + 6, itemY + 2, itemWidth - 20, COLOR_AMBER_BRIGHT);
            
            // Number badge
            tft.fillRoundRect(MARGIN + 6, itemY + 5, 24, 20, 3, COLOR_TEXT_DARK);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setTextColor(COLOR_AMBER);
            tft.setCursor(MARGIN + 10, itemY + 8);
            if (alertIndex + 1 < 10) tft.print(' ');
            tft.print(alertIndex + 1);
            
            // Alert name
            tft.setTextColor(COLOR_TEXT_DARK);
            tft.setCursor(MARGIN + 36, itemY + 8);
            tft.print(alertNamesShort[alertIndex]);
            
            // Priority dot with border
            uint16_t prioColor = getAlertColor(alertIndex);
            if (prioColor == COLOR_AMBER) prioColor = COLOR_ORANGE_DARK;
            tft.fillCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 6, prioColor);
            tft.drawCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 7, COLOR_TEXT_DARK);
            tft.drawCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 8, COLOR_TEXT_DARK);
            
        } else {
            // ─── NON-SELECTED ITEM: Subtle card style ───
            // Subtle card border
            tft.drawRoundRect(MARGIN, itemY, itemWidth, itemHeight - 3, 4, COLOR_BORDER);
            
            // Number badge
            tft.fillRoundRect(MARGIN + 4, itemY + 6, 22, 18, 3, COLOR_BG_CARD);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setTextColor(COLOR_TEXT_MUTED);
            tft.setCursor(MARGIN + 8, itemY + 9);
            if (alertIndex + 1 < 10) tft.print(' ');
            tft.print(alertIndex + 1);
            
            // Alert name
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(MARGIN + 32, itemY + 9);
            tft.print(alertNamesShort[alertIndex]);
            
            // Priority indicator
            uint16_t prioColor = getAlertColor(alertIndex);
            tft.fillCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 15, 5, prioColor);
        }
    }
    
    // ─────────────────── PREMIUM FOOTER ───────────────────
    int footerY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    tft.drawFastHLine(0, footerY - 2, SCREEN_WIDTH, COLOR_CYAN_DARK);
    tft.drawFastHLine(0, footerY - 1, SCREEN_WIDTH, COLOR_ACCENT_LINE);
    drawGradientV(0, footerY, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_BG_HEADER_ALT, RGB565(5, 10, 15));
    
    // Key hints with badge style
    int hintY = footerY + (FOOTER_HEIGHT - 8) / 2;
    int hintX = MARGIN;
    
    drawKeyBadge(hintX, hintY, 'A', "", COLOR_CYAN);
    tft.print((char)24);
    hintX += 22;
    
    drawKeyBadge(hintX, hintY, 'B', "", COLOR_CYAN);
    tft.print((char)25);
    hintX += 26;
    
    tft.drawFastVLine(hintX, footerY + 6, FOOTER_HEIGHT - 12, COLOR_ACCENT_LINE);
    hintX += 6;
    
    drawKeyBadge(hintX, hintY, '*', "SEL", COLOR_GREEN);
    hintX += 44;
    
    tft.drawFastVLine(hintX, footerY + 6, FOOTER_HEIGHT - 12, COLOR_ACCENT_LINE);
    hintX += 6;
    
    drawKeyBadge(hintX, hintY, 'C', "Info", COLOR_TEXT_SECONDARY);
    hintX += 44;
    
    drawKeyBadge(hintX, hintY, 'D', "Help", COLOR_TEXT_SECONDARY);
    
    Serial.println(F("[SCREEN] Premium Menu displayed"));
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                        3. ALERT CONFIRMATION SCREEN
//     Purpose: Prevent accidental or false transmissions
//     Design: Clear warning, large alert display, prominent SEND/CANCEL buttons
// ═══════════════════════════════════════════════════════════════════════════════════

void drawConfirmScreen() {
    // INSTANT TRANSITION - Draw header first, then content area
    drawGradientV(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGB565(60, 45, 0), COLOR_AMBER_DARK);
    
    // Fill content area below header - instant overwrite
    tft.fillRect(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - 1, COLOR_BG_PRIMARY);
    
    // Accent lines below header
    tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_AMBER_BRIGHT);
    tft.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_AMBER);
    
    // Warning icon (premium triangle with glow)
    int triX = MARGIN + 20;
    int triCenterY = HEADER_HEIGHT / 2;
    
    // Glow effect layers
    for (int g = 3; g > 0; g--) {
        uint16_t glowColor = RGB565(30 - g*8, 25 - g*6, 0);
        tft.fillTriangle(triX, triCenterY - 11 - g, triX - 11 - g, triCenterY + 7 + g, triX + 11 + g, triCenterY + 7 + g, glowColor);
    }
    
    // Main warning triangle
    tft.fillTriangle(triX, triCenterY - 9, triX - 9, triCenterY + 6, triX + 9, triCenterY + 6, COLOR_TEXT_DARK);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_AMBER);
    tft.setCursor(triX - 5, triCenterY - 5);
    tft.print('!');
    
    // Header title - properly positioned
    int headerTextX = MARGIN + 42;
    int headerTextY = (HEADER_HEIGHT - 14) / 2;
    tft.setTextSize(TEXT_MEDIUM);
    // Shadow
    tft.setTextColor(RGB565(40, 30, 0));
    tft.setCursor(headerTextX + 1, headerTextY + 1);
    tft.print(F("CONFIRM TRANSMISSION"));
    // Main text
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(headerTextX, headerTextY);
    tft.print(F("CONFIRM TRANSMISSION"));
    
    // ─────────────────── ALERT TYPE LABEL ───────────────────
    int contentY = HEADER_HEIGHT + 12;
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN, contentY);
    tft.print(F("SELECTED ALERT"));
    
    // Decorative line - adjusted position
    tft.drawFastHLine(MARGIN + 92, contentY + 4, SCREEN_WIDTH - MARGIN * 2 - 92, COLOR_ACCENT_LINE);
    
    // ─────────────────── PREMIUM ALERT DISPLAY CARD ───────────────────
    int alertBoxY = contentY + 18;
    int alertBoxH = 58;
    int alertBoxW = SCREEN_WIDTH - (2 * MARGIN);
    uint16_t alertColor = getAlertColor(selectedAlertIndex);
    
    // Card shadow
    tft.fillRoundRect(MARGIN + 3, alertBoxY + 3, alertBoxW, alertBoxH, 6, RGB565(5, 5, 10));
    
    // Main card background
    tft.fillRoundRect(MARGIN, alertBoxY, alertBoxW, alertBoxH, 6, COLOR_BG_CARD);
    
    // Glowing color indicator bar on left
    for (int g = 3; g >= 0; g--) {
        uint16_t barColor = (g == 0) ? alertColor : RGB565(
            ((alertColor >> 11) & 0x1F) / (g + 1),
            ((alertColor >> 5) & 0x3F) / (g + 1),
            (alertColor & 0x1F) / (g + 1)
        );
        tft.fillRect(MARGIN, alertBoxY, 6 + g, alertBoxH, barColor);
    }
    
    // Glass highlight on card top
    tft.drawFastHLine(MARGIN + 12, alertBoxY + 2, alertBoxW - 24, RGB565(50, 50, 70));
    
    // Alert name (large, colored) - with proper positioning
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(alertColor);
    tft.setCursor(MARGIN + 18, alertBoxY + 12);
    tft.print(alertNames[selectedAlertIndex]);
    
    // Alert code badge - right aligned inside card
    int codeBadgeW = 34;
    int codeBadgeH = 20;
    int codeBadgeX = SCREEN_WIDTH - MARGIN - codeBadgeW - 8;
    int codeBadgeY = alertBoxY + 10;
    tft.fillRoundRect(codeBadgeX, codeBadgeY, codeBadgeW, codeBadgeH, 4, COLOR_BG_INPUT);
    tft.drawRoundRect(codeBadgeX, codeBadgeY, codeBadgeW, codeBadgeH, 4, alertColor);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(alertColor);
    tft.setCursor(codeBadgeX + 10, codeBadgeY + 3);
    tft.print(getAlertCode(selectedAlertIndex));
    
    // Priority label - bottom of alert card
    const char* prioLabels[] = {"CRITICAL", "HIGH", "MEDIUM", "OK", "INFO"};
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 18, alertBoxY + 38);
    tft.print(F("Priority: "));
    tft.setTextColor(alertColor);
    tft.print(prioLabels[min((int)alertPriority[selectedAlertIndex], 4)]);
    
    // ─────────────────── DEVICE INFO CARD ───────────────────
    int deviceInfoY = alertBoxY + alertBoxH + 10;
    int deviceInfoH = 36;
    drawPremiumCard(MARGIN, deviceInfoY, alertBoxW, deviceInfoH, COLOR_BG_CARD, COLOR_BORDER);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 12, deviceInfoY + 8);
    tft.print(F("FROM DEVICE:"));
    
    // Device name on same card
    tft.setTextColor(COLOR_CYAN);
    char devStr[24];
    sprintf(devStr, "TX Unit #%03d", DEVICE_ID);
    tft.setCursor(MARGIN + 12, deviceInfoY + 20);
    tft.print(devStr);
    
    // LoRa status indicator - right side
    int loraIndicatorX = SCREEN_WIDTH - MARGIN - 22;
    int loraIndicatorY = deviceInfoY + deviceInfoH / 2;
    tft.fillCircle(loraIndicatorX, loraIndicatorY, 6, loraInitialized ? COLOR_GREEN : COLOR_RED);
    
    // ─────────────────── PREMIUM ACTION BUTTONS ───────────────────
    int btnY = SCREEN_HEIGHT - 54;
    int btnW = 105;
    int btnH = 38;
    int btnGap = 20;
    int totalBtnWidth = (btnW * 2) + btnGap;
    int btnStartX = (SCREEN_WIDTH - totalBtnWidth) / 2;
    
    // SEND button with glow effect
    for (int g = 2; g >= 0; g--) {
        uint16_t glowColor = RGB565(0, 50 - g*15, 25 - g*8);
        tft.fillRoundRect(btnStartX - g, btnY - g, btnW + g*2, btnH + g*2, 6, glowColor);
    }
    tft.fillRoundRect(btnStartX, btnY, btnW, btnH, 5, COLOR_GREEN);
    tft.drawFastHLine(btnStartX + 6, btnY + 3, btnW - 12, COLOR_GREEN_BRIGHT);  // Highlight
    
    // SEND button text - centered
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_DARK);
    int16_t tx1, ty1;
    uint16_t tw1, th1;
    tft.getTextBounds("* SEND", 0, 0, &tx1, &ty1, &tw1, &th1);
    tft.setCursor(btnStartX + (btnW - tw1) / 2, btnY + (btnH - th1) / 2);
    tft.print(F("* SEND"));
    
    // CANCEL button (outline style)
    int cancelX = btnStartX + btnW + btnGap;
    tft.fillRoundRect(cancelX, btnY, btnW, btnH, 5, COLOR_BG_CARD);
    tft.drawRoundRect(cancelX, btnY, btnW, btnH, 5, COLOR_RED);
    tft.drawRoundRect(cancelX + 1, btnY + 1, btnW - 2, btnH - 2, 4, COLOR_RED_DARK);
    
    // CANCEL button text - centered
    tft.setTextColor(COLOR_RED);
    tft.getTextBounds("# CANCEL", 0, 0, &tx1, &ty1, &tw1, &th1);
    tft.setCursor(cancelX + (btnW - tw1) / 2, btnY + (btnH - th1) / 2);
    tft.print(F("# CANCEL"));
    
    Serial.println(F("[SCREEN] Confirm screen displayed"));
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                        4. TRANSMISSION STATUS SCREEN
//     Purpose: Operator confidence and system feedback
//     Shows: Transmission progress, then result (success/failure)
// ═══════════════════════════════════════════════════════════════════════════════════

void drawSendingScreen() {
    // INSTANT TRANSITION - Draw header first
    drawGradientV(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGB565(15, 35, 55), COLOR_BG_HEADER);
    
    // Fill content area with gradient - overwrites previous content instantly
    drawGradientV(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - 1, RGB565(8, 15, 25), COLOR_BG_PRIMARY);
    tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_CYAN_BRIGHT);
    tft.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_CYAN);
    
    // Header text
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(MARGIN, (HEADER_HEIGHT - 14) / 2);
    tft.print(F("TRANSMITTING"));
    
    // Animated dots - fixed position after text
    int dotsStartX = MARGIN + 130;
    for (int d = 0; d < 3; d++) {
        tft.fillCircle(dotsStartX + d * 12, HEADER_HEIGHT / 2, 3, COLOR_CYAN);
    }
    
    // ─────────────────── PREMIUM RADIO WAVES ANIMATION ───────────────────
    int cx = SCREEN_WIDTH / 2;
    int cy = 90;
    
    // Outer glow rings (static concentric circles)
    for (int i = 4; i > 0; i--) {
        uint16_t ringColor = RGB565(80 - i*15, 40 - i*8, 10);
        int ringRadius = 18 + (i * 14);
        tft.drawCircle(cx, cy, ringRadius, ringColor);
        tft.drawCircle(cx, cy, ringRadius + 1, ringColor);
    }
    
    // Central transmitter icon with glow
    drawGlowCircle(cx, cy, 16, COLOR_ORANGE);
    
    // Antenna on top of circle
    tft.fillRect(cx - 3, cy - 26, 6, 12, COLOR_ORANGE);
    tft.fillCircle(cx, cy - 28, 5, COLOR_ORANGE);
    
    // Signal direction arrows - left and right
    tft.setTextColor(COLOR_ORANGE);
    tft.setTextSize(TEXT_SMALL);
    tft.setCursor(cx - 55, cy - 3);
    tft.print(F(">>>"));
    tft.setCursor(cx + 28, cy - 3);
    tft.print(F(">>>"));
    
    // ─────────────────── ALERT INFO CARD ───────────────────
    int alertCardY = 145;
    int alertCardH = 54;
    int alertCardW = SCREEN_WIDTH - MARGIN * 2;
    
    drawPremiumCard(MARGIN, alertCardY, alertCardW, alertCardH, COLOR_BG_CARD, COLOR_ORANGE_DARK);
    
    // Left accent bar
    tft.fillRect(MARGIN, alertCardY, 5, alertCardH, COLOR_ORANGE);
    
    // Label
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 14, alertCardY + 10);
    tft.print(F("SENDING ALERT:"));
    
    // Alert name - bold
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN + 14, alertCardY + 28);
    tft.print(alertNamesShort[selectedAlertIndex]);
    
    // Alert code badge - right side
    uint16_t alertColor = getAlertColor(selectedAlertIndex);
    int sendBadgeW = 32;
    int sendBadgeH = 24;
    int sendBadgeX = SCREEN_WIDTH - MARGIN - sendBadgeW - 10;
    int sendBadgeY = alertCardY + (alertCardH - sendBadgeH) / 2;
    tft.fillRoundRect(sendBadgeX, sendBadgeY, sendBadgeW, sendBadgeH, 4, alertColor);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(sendBadgeX + 9, sendBadgeY + 5);
    tft.print(getAlertCode(selectedAlertIndex));
    
    // ─────────────────── DEVICE INFO ───────────────────
    int deviceInfoY = alertCardY + alertCardH + 10;
    char devStr[24];
    sprintf(devStr, "From TX Unit #%03d", DEVICE_ID);
    drawCenteredText(devStr, deviceInfoY, TEXT_SMALL, COLOR_TEXT_MUTED);
    
    // ─────────────────── PROGRESS INDICATOR ───────────────────
    int progressY = deviceInfoY + 20;
    int progressW = SCREEN_WIDTH - MARGIN * 4;
    int progressX = (SCREEN_WIDTH - progressW) / 2;
    int progressH = 8;
    
    // Progress bar background
    tft.fillRoundRect(progressX, progressY, progressW, progressH, 3, COLOR_BG_CARD);
    
    // Animated progress fill (static snapshot)
    int fillW = progressW / 2;  // Show 50% for static display
    tft.fillRoundRect(progressX + 1, progressY + 1, fillW, progressH - 2, 2, COLOR_ORANGE);
    
    Serial.println(F("[SCREEN] Premium Sending screen displayed"));
}

/**
 * Draw premium transmission result screen
 */
void drawResultScreen() {
    if (lastTransmitSuccess) {
        // ═══════════════════ SUCCESS SCREEN ═══════════════════
        // INSTANT TRANSITION - Draw header first
        drawGradientV(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGB565(15, 60, 35), COLOR_GREEN_DARK);
        
        // Fill content area with gradient - instant overwrite
        drawGradientV(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - 1, RGB565(10, 40, 25), RGB565(5, 20, 12));
        tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_GREEN_BRIGHT);
        tft.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_GREEN);
        
        // Header text
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(MARGIN, (HEADER_HEIGHT - 14) / 2);
        tft.print(F("TRANSMISSION SUCCESS"));
        
        // Large glowing checkmark icon
        int cx = SCREEN_WIDTH / 2;
        int cy = 92;
        int circleRadius = 30;
        
        // Outer glow rings
        for (int g = 4; g > 0; g--) {
            uint16_t glowColor = RGB565(0, 70 - g*14, 35 - g*7);
            tft.drawCircle(cx, cy, circleRadius + g*3, glowColor);
        }
        
        // Main circle
        tft.fillCircle(cx, cy, circleRadius, COLOR_GREEN);
        
        // Inner highlight
        tft.drawCircle(cx - 4, cy - 4, circleRadius - 4, COLOR_GREEN_BRIGHT);
        
        // Thick checkmark - properly centered
        for (int t = -3; t <= 3; t++) {
            tft.drawLine(cx - 14, cy + t, cx - 4, cy + 12 + t, COLOR_TEXT_DARK);
            tft.drawLine(cx - 4, cy + 12 + t, cx + 16, cy - 10 + t, COLOR_TEXT_DARK);
        }
        
        // Success message card
        int successCardY = 138;
        int successCardH = 50;
        int successCardW = SCREEN_WIDTH - MARGIN * 2;
        drawPremiumCard(MARGIN, successCardY, successCardW, successCardH, COLOR_BG_CARD, COLOR_GREEN_DARK);
        tft.fillRect(MARGIN, successCardY, 5, successCardH, COLOR_GREEN);
        
        // "Message Sent" text
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.setCursor(MARGIN + 16, successCardY + 10);
        tft.print(F("Message Sent!"));
        
        // Alert name
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(MARGIN + 16, successCardY + 32);
        tft.print(F("Alert: "));
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.print(alertNamesShort[selectedAlertIndex]);
        
        // Auto-return indicator at bottom
        int autoReturnY = SCREEN_HEIGHT - 38;
        int autoReturnH = 26;
        tft.fillRoundRect(MARGIN * 2, autoReturnY, SCREEN_WIDTH - MARGIN * 4, autoReturnH, 4, RGB565(15, 30, 20));
        drawCenteredText("Returning to menu...", autoReturnY + 9, TEXT_SMALL, COLOR_TEXT_MUTED);
        
        setLED(LED_GREEN, true);
        setLED(LED_RED, false);
        playSuccessTone();
        
    } else {
        // ═══════════════════ FAILURE SCREEN ═══════════════════
        // INSTANT TRANSITION - Draw header first
        drawGradientV(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGB565(70, 20, 20), COLOR_RED_DARK);
        
        // Fill content area with gradient - instant overwrite
        drawGradientV(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - 1, RGB565(45, 15, 15), RGB565(20, 8, 8));
        tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_RED_BRIGHT);
        tft.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_RED);
        
        // Header text
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(MARGIN, (HEADER_HEIGHT - 14) / 2);
        tft.print(F("TRANSMISSION FAILED"));
        
        // Large glowing X icon
        int cx = SCREEN_WIDTH / 2;
        int cy = 88;
        int circleRadius = 30;
        
        // Outer glow rings
        for (int g = 4; g > 0; g--) {
            uint16_t glowColor = RGB565(70 - g*14, 18 - g*3, 18 - g*3);
            tft.drawCircle(cx, cy, circleRadius + g*3, glowColor);
        }
        
        // Main circle - using CYAN for better visibility (as shown in image)
        tft.fillCircle(cx, cy, circleRadius, COLOR_CYAN);
        
        // Thick X mark - properly centered
        int xSize = 14;
        for (int t = -3; t <= 3; t++) {
            tft.drawLine(cx - xSize + t, cy - xSize, cx + xSize + t, cy + xSize, COLOR_TEXT_PRIMARY);
            tft.drawLine(cx + xSize + t, cy - xSize, cx - xSize + t, cy + xSize, COLOR_TEXT_PRIMARY);
        }
        
        // Failure message - centered
        drawCenteredText("Send Failed!", 138, TEXT_LARGE, COLOR_CYAN);
        
        // Retry info card
        int retryCardY = 166;
        int retryCardH = 32;
        int retryCardW = SCREEN_WIDTH - MARGIN * 2;
        drawPremiumCard(MARGIN, retryCardY, retryCardW, retryCardH, COLOR_BG_CARD, COLOR_BORDER);
        
        char retryStr[32];
        sprintf(retryStr, "Attempt %d of %d failed", retryCount + 1, MAX_RETRY_ATTEMPTS);
        drawCenteredText(retryStr, retryCardY + 10, TEXT_SMALL, COLOR_TEXT_SECONDARY);
        
        // ─────────────────── ACTION BUTTONS ───────────────────
        int failBtnY = SCREEN_HEIGHT - 52;
        int failBtnW = 100;
        int failBtnH = 36;
        int failBtnGap = 20;
        int failBtnTotalW = (failBtnW * 2) + failBtnGap;
        int failBtnStartX = (SCREEN_WIDTH - failBtnTotalW) / 2;
        
        // RETRY button with glow
        for (int g = 2; g >= 0; g--) {
            uint16_t glowColor = RGB565(50 - g*15, 40 - g*12, 0);
            tft.fillRoundRect(failBtnStartX - g, failBtnY - g, failBtnW + g*2, failBtnH + g*2, 5, glowColor);
        }
        tft.fillRoundRect(failBtnStartX, failBtnY, failBtnW, failBtnH, 5, COLOR_AMBER);
        tft.drawFastHLine(failBtnStartX + 6, failBtnY + 3, failBtnW - 12, COLOR_AMBER_BRIGHT);
        
        // RETRY button text - centered
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_TEXT_DARK);
        int16_t rtx, rty;
        uint16_t rtw, rth;
        tft.getTextBounds("* RETRY", 0, 0, &rtx, &rty, &rtw, &rth);
        tft.setCursor(failBtnStartX + (failBtnW - rtw) / 2, failBtnY + (failBtnH - rth) / 2);
        tft.print(F("* RETRY"));
        
        // MENU button
        int menuBtnX = failBtnStartX + failBtnW + failBtnGap;
        tft.fillRoundRect(menuBtnX, failBtnY, failBtnW, failBtnH, 5, COLOR_BG_CARD);
        tft.drawRoundRect(menuBtnX, failBtnY, failBtnW, failBtnH, 5, COLOR_TEXT_SECONDARY);
        
        // MENU button text - centered
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.getTextBounds("# MENU", 0, 0, &rtx, &rty, &rtw, &rth);
        tft.setCursor(menuBtnX + (failBtnW - rtw) / 2, failBtnY + (failBtnH - rth) / 2);
        tft.print(F("# MENU"));
        
        setLED(LED_GREEN, false);
        setLED(LED_RED, true);
        playErrorTone();
    }
    
    resultStartTime = millis();
    Serial.printf("[SCREEN] Premium Result: %s\n", lastTransmitSuccess ? "SUCCESS" : "FAILED");
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                      5. SYSTEM INFORMATION SCREEN (Key C)
//     Purpose: Trust & diagnostics - rugged device diagnostics screen
//     Shows: Device info, LoRa status, battery, transmission stats
// ═══════════════════════════════════════════════════════════════════════════════════

void drawSystemInfoScreen() {
    // INSTANT TRANSITION - Draw header first (covers top), then fill content area only
    drawHeader("SYSTEM INFORMATION");
    
    // Fill only content area (below header) - faster than full screen clear
    tft.fillRect(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - 1, COLOR_BG_PRIMARY);
    
    // ─────────────────── INFO SECTIONS WITH CARDS ───────────────────
    int y = HEADER_HEIGHT + 10;
    int cardX = MARGIN;
    int cardW = SCREEN_WIDTH - MARGIN * 2;
    int cardSpacing = 8;
    
    // ═══════════════════ DEVICE CARD ═══════════════════
    int deviceCardH = 52;
    drawPremiumCard(cardX, y, cardW, deviceCardH, COLOR_BG_CARD, COLOR_CYAN_DARK);
    
    // Section icon
    int iconCenterY = y + deviceCardH / 2;
    tft.fillCircle(cardX + 18, iconCenterY, 10, COLOR_CYAN);
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setTextSize(TEXT_SMALL);
    tft.setCursor(cardX + 14, iconCenterY - 3);
    tft.print(F("D"));
    
    // Device info - label
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(cardX + 36, y + 10);
    tft.print(F("DEVICE"));
    
    // Device name
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(cardX + 36, y + 26);
    tft.print(DEVICE_NAME);
    
    // Device ID badge - right aligned
    char idStr[12];
    sprintf(idStr, "#%03d", DEVICE_ID);
    int badgeW = 48;
    int badgeH = 24;
    int badgeX = cardX + cardW - badgeW - 10;
    int badgeY = y + (deviceCardH - badgeH) / 2;
    tft.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 4, COLOR_BG_INPUT);
    tft.drawRoundRect(badgeX, badgeY, badgeW, badgeH, 4, COLOR_CYAN);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(badgeX + 6, badgeY + 5);
    tft.print(idStr);
    
    y += deviceCardH + cardSpacing;
    
    // ═══════════════════ LORA STATUS CARD ═══════════════════
    int loraCardH = 62;
    uint16_t loraBorderColor = loraInitialized ? COLOR_GREEN_DARK : COLOR_RED_DARK;
    drawPremiumCard(cardX, y, cardW, loraCardH, COLOR_BG_CARD, loraBorderColor);
    
    // Status indicator with glow - positioned left
    int indicatorX = cardX + 18;
    int indicatorCenterY = y + loraCardH / 2;
    uint16_t statusColor = loraInitialized ? COLOR_GREEN : COLOR_RED;
    
    // Glow effect
    for (int g = 3; g > 0; g--) {
        uint16_t glowColor = loraInitialized ? 
            RGB565(0, 40 - g*10, 20 - g*5) : 
            RGB565(40 - g*10, 10 - g*2, 10 - g*2);
        tft.drawCircle(indicatorX, indicatorCenterY, 9 + g, glowColor);
    }
    tft.fillCircle(indicatorX, indicatorCenterY, 9, statusColor);
    tft.fillCircle(indicatorX - 2, indicatorCenterY - 2, 2, COLOR_TEXT_PRIMARY);  // Highlight
    
    // LoRa info - section label
    int loraTextX = cardX + 38;
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(loraTextX, y + 8);
    tft.print(F("LORA RADIO"));
    
    // Status text - CONNECTED or ERROR
    tft.setTextColor(loraInitialized ? COLOR_GREEN : COLOR_RED);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(loraTextX, y + 24);
    tft.print(loraInitialized ? F("CONNECTED") : F("ERROR"));
    
    // Frequency info on separate line
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(loraTextX, y + 44);
    char freqStr[24];
    sprintf(freqStr, "%.1f MHz | SF%d", LORA_FREQUENCY / 1E6, LORA_SF);
    tft.print(freqStr);
    
    // Last TX indicator - right side
    int txStatusX = cardX + cardW - 55;
    tft.setCursor(txStatusX, y + 44);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.print(F("TX: "));
    if (totalTransmissions > 0) {
        tft.setTextColor(lastTransmitSuccess ? COLOR_GREEN : COLOR_RED);
        tft.print(lastTransmitSuccess ? F("OK") : F("--"));
    } else {
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.print(F("--"));
    }
    
    y += loraCardH + cardSpacing;
    
    // ═══════════════════ SYSTEM STATS CARD ═══════════════════
    int sysCardH = 54;
    drawPremiumCard(cardX, y, cardW, sysCardH, COLOR_BG_CARD, COLOR_PURPLE_DARK);
    
    // Section label
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_PURPLE);
    tft.setCursor(cardX + 12, y + 8);
    tft.print(F("SYSTEM"));
    
    // Power status - first data row
    int dataRow1Y = y + 22;
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(cardX + 12, dataRow1Y);
    tft.print(F("Power:"));
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(cardX + 58, dataRow1Y);
    if (batteryPercent >= 0) {
        tft.print(batteryPercent);
        tft.print(F("%"));
    } else {
        tft.print(F("USB"));
    }
    
    // Firmware version - second data row
    int dataRow2Y = y + 38;
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(cardX + 12, dataRow2Y);
    tft.print(F("FW:"));
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(cardX + 36, dataRow2Y);
    tft.print(FIRMWARE_VERSION);
    
    // TX Stats - right column
    int statsX = cardX + cardW - 95;
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(statsX, dataRow1Y);
    tft.print(F("Sent:"));
    
    // Stats value on same line
    char statsStr[16];
    sprintf(statsStr, "%d/%d", successfulTransmissions, totalTransmissions);
    tft.setTextColor(COLOR_AMBER);
    tft.setCursor(statsX + 38, dataRow1Y);
    tft.print(statsStr);
    
    // ─────────────────── PREMIUM FOOTER ───────────────────
    drawFooter("Press any key to return");
    
    Serial.println(F("[SCREEN] Premium System Info displayed"));
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                     6. USER MANUAL / HELP SCREEN (Key D)
//     Purpose: Zero-training usability with chunked instructional blocks
//     Design: Short bullet blocks, page navigation, no long paragraphs
// ═══════════════════════════════════════════════════════════════════════════════════

void drawUserManualScreen() {
    // INSTANT TRANSITION - Draw header first
    drawHeader("USER MANUAL");
    
    // Fill content area only - instant overwrite
    tft.fillRect(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 2, COLOR_BG_PRIMARY);
    
    // Page indicator badge in header
    char pageStr[10];
    sprintf(pageStr, "%d/%d", manualPage + 1, MANUAL_TOTAL_PAGES);
    int badgeW = 36;
    int badgeX = SCREEN_WIDTH - MARGIN - badgeW;
    tft.fillRoundRect(badgeX, 10, badgeW, 18, 4, COLOR_BG_CARD);
    tft.drawRoundRect(badgeX, 10, badgeW, 18, 4, COLOR_PURPLE);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_PURPLE);
    tft.setCursor(badgeX + 8, 14);
    tft.print(pageStr);
    
    // ─────────────────── CONTENT CARD ───────────────────
    int cardY = HEADER_HEIGHT + 6;
    int cardH = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 14;
    drawPremiumCard(MARGIN, cardY, SCREEN_WIDTH - MARGIN * 2, cardH, COLOR_BG_CARD, COLOR_BORDER);
    
    int y = cardY + 10;
    int contentX = MARGIN + 12;
    
    tft.setTextSize(TEXT_SMALL);
    
    switch (manualPage) {
        case 0:  // Page 1: Selecting Alerts
            // Section header with icon
            tft.fillRoundRect(contentX - 4, y, 28, 20, 4, COLOR_AMBER);
            tft.setTextColor(COLOR_TEXT_DARK);
            tft.setCursor(contentX, y + 6);
            tft.print(F("123"));
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setCursor(contentX + 36, y + 3);
            tft.print(F("SELECT ALERTS"));
            y += 28;
            
            // Decorative line
            tft.drawFastHLine(contentX, y, SCREEN_WIDTH - MARGIN * 2 - 24, COLOR_AMBER_DARK);
            y += 10;
            
            tft.setTextSize(TEXT_SMALL);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX, y);
            tft.print(F("Use number keys"));
            tft.setTextColor(COLOR_AMBER);
            tft.print(F(" 1-9 "));
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.print(F("for"));
            y += 14;
            tft.setCursor(contentX, y);
            tft.print(F("instant selection."));
            y += 20;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX, y);
            tft.print(F("Press"));
            tft.setTextColor(COLOR_AMBER);
            tft.print(F(" 0 "));
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.print(F("for alert #10"));
            y += 22;
            
            // Tip box
            tft.fillRoundRect(contentX - 4, y, SCREEN_WIDTH - MARGIN * 2 - 16, 42, 4, RGB565(30, 30, 20));
            tft.drawRoundRect(contentX - 4, y, SCREEN_WIDTH - MARGIN * 2 - 16, 42, 4, COLOR_CYAN_DARK);
            
            tft.setTextColor(COLOR_CYAN);
            tft.setCursor(contentX + 4, y + 8);
            tft.print(F("TIP:"));
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.print(F(" Yellow highlight"));
            tft.setCursor(contentX + 4, y + 24);
            tft.print(F("shows current selection"));
            break;
            
        case 1:  // Page 2: Navigation
            // Section header
            tft.fillRoundRect(contentX - 4, y, 28, 20, 4, COLOR_CYAN);
            tft.setTextColor(COLOR_TEXT_DARK);
            tft.setCursor(contentX + 2, y + 6);
            tft.print((char)24);
            tft.print((char)25);
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setCursor(contentX + 36, y + 3);
            tft.print(F("NAVIGATION"));
            y += 28;
            
            tft.drawFastHLine(contentX, y, SCREEN_WIDTH - MARGIN * 2 - 24, COLOR_CYAN_DARK);
            y += 12;
            
            tft.setTextSize(TEXT_SMALL);
            
            // Key explanations with badges
            drawKeyBadge(contentX, y + 2, 'A', " Scroll UP", COLOR_CYAN);
            y += 20;
            
            drawKeyBadge(contentX, y + 2, 'B', " Scroll DOWN", COLOR_CYAN);
            y += 24;
            
            tft.drawFastHLine(contentX, y, 80, COLOR_ACCENT_LINE);
            y += 8;
            
            drawKeyBadge(contentX, y + 2, 'C', " System Info", COLOR_PURPLE);
            y += 20;
            
            drawKeyBadge(contentX, y + 2, 'D', " This Help", COLOR_PURPLE);
            y += 22;
            
            tft.setTextColor(COLOR_TEXT_MUTED);
            tft.setCursor(contentX, y);
            tft.print(F("Menu wraps at top/bottom"));
            break;
            
        case 2:  // Page 3: Sending Alerts
            // Section header
            tft.fillRoundRect(contentX - 4, y, 28, 20, 4, COLOR_GREEN);
            tft.setTextColor(COLOR_TEXT_DARK);
            tft.setCursor(contentX + 2, y + 6);
            tft.print(F("TX"));
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setCursor(contentX + 36, y + 3);
            tft.print(F("SENDING"));
            y += 28;
            
            tft.drawFastHLine(contentX, y, SCREEN_WIDTH - MARGIN * 2 - 24, COLOR_GREEN_DARK);
            y += 12;
            
            tft.setTextSize(TEXT_SMALL);
            
            drawKeyBadge(contentX, y + 2, '*', " Confirm & Send", COLOR_GREEN);
            y += 24;
            
            drawKeyBadge(contentX, y + 2, '#', " Cancel & Back", COLOR_RED);
            y += 26;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX, y);
            tft.print(F("Confirmation screen"));
            y += 12;
            tft.setCursor(contentX, y);
            tft.print(F("appears before sending"));
            y += 18;
            
            // Warning box
            tft.fillRoundRect(contentX - 4, y, SCREEN_WIDTH - MARGIN * 2 - 16, 26, 4, COLOR_RED_DARK);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(contentX + 4, y + 8);
            tft.print(F("Failed? Press * to retry"));
            break;
            
        case 3:  // Page 4: LED Indicators
            // Section header
            tft.fillRoundRect(contentX - 4, y, 28, 20, 4, COLOR_PURPLE);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(contentX, y + 6);
            tft.print(F("LED"));
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setCursor(contentX + 36, y + 3);
            tft.print(F("INDICATORS"));
            y += 28;
            
            tft.drawFastHLine(contentX, y, SCREEN_WIDTH - MARGIN * 2 - 24, COLOR_PURPLE_DARK);
            y += 12;
            
            tft.setTextSize(TEXT_SMALL);
            
            // Green LED with glow
            drawStatusIndicator(contentX + 8, y + 6, 6, COLOR_GREEN);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX + 22, y + 2);
            tft.print(F("GREEN = Sent OK"));
            y += 22;
            
            // Red LED with glow
            drawStatusIndicator(contentX + 8, y + 6, 6, COLOR_RED);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX + 22, y + 2);
            tft.print(F("RED = Send Failed"));
            y += 28;
            
            // Emergency tip box
            tft.fillRoundRect(contentX - 4, y, SCREEN_WIDTH - MARGIN * 2 - 16, 52, 4, RGB565(35, 30, 15));
            tft.drawRoundRect(contentX - 4, y, SCREEN_WIDTH - MARGIN * 2 - 16, 52, 4, COLOR_AMBER_DARK);
            
            tft.setTextColor(COLOR_AMBER);
            tft.setCursor(contentX + 4, y + 6);
            tft.print(F("!! EMERGENCY TIP !!"));
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX + 4, y + 20);
            tft.print(F("Only send alerts when"));
            tft.setCursor(contentX + 4, y + 34);
            tft.print(F("necessary."));
            break;
    }
    
    // ─────────────────── PREMIUM FOOTER ───────────────────
    int footerY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    tft.drawFastHLine(0, footerY - 2, SCREEN_WIDTH, COLOR_CYAN_DARK);
    tft.drawFastHLine(0, footerY - 1, SCREEN_WIDTH, COLOR_ACCENT_LINE);
    drawGradientV(0, footerY, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_BG_HEADER_ALT, RGB565(5, 10, 15));
    
    int hintY = footerY + (FOOTER_HEIGHT - 8) / 2;
    
    // Navigation hints with badges
    drawKeyBadge(MARGIN, hintY, 'A', "Prev", manualPage > 0 ? COLOR_CYAN : COLOR_TEXT_DISABLED);
    drawKeyBadge(MARGIN + 50, hintY, 'B', "Next", manualPage < MANUAL_TOTAL_PAGES - 1 ? COLOR_CYAN : COLOR_TEXT_DISABLED);
    
    tft.drawFastVLine(MARGIN + 100, footerY + 6, FOOTER_HEIGHT - 12, COLOR_ACCENT_LINE);
    
    drawKeyBadge(MARGIN + 110, hintY, '#', "Back", COLOR_TEXT_SECONDARY);
    
    Serial.printf("[SCREEN] Premium Manual page %d displayed\n", manualPage + 1);
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              LORA TRANSMISSION
// ═══════════════════════════════════════════════════════════════════════════════════

bool transmitAlert() {
    if (!loraInitialized) {
        Serial.println(F("[LORA] ERROR: Not initialized"));
        return false;
    }
    
    char alertCode = getAlertCode(selectedAlertIndex);
    char packet[16];
    sprintf(packet, "TX%03d,%c", DEVICE_ID, alertCode);
    
    Serial.printf("[LORA] TX: %s (%s)\n", packet, alertNames[selectedAlertIndex]);
    
    // Ensure LoRa is in idle state before transmitting
    LoRa.idle();
    delay(10);
    
    LoRa.beginPacket();
    LoRa.print(packet);
    bool success = LoRa.endPacket();  // Synchronous mode - wait for TX complete
    
    // Small delay to ensure radio returns to idle
    delay(50);
    
    totalTransmissions++;
    if (success) {
        successfulTransmissions++;
        lastTransmitTime = millis();
    }
    
    Serial.printf("[LORA] Result: %s\n", success ? "OK" : "FAILED");
    return success;
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              INPUT HANDLING
// ═══════════════════════════════════════════════════════════════════════════════════

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
    
    // Direct number selection - INSTANT transition to confirm
    if (key >= '1' && key <= '9') {
        int targetIndex = key - '1';
        if (targetIndex < ALERT_COUNT) {
            selectedAlertIndex = targetIndex;
            // INSTANT: Skip menu redraw, go straight to confirm
            previousScreen = SCREEN_MENU;
            currentScreen = SCREEN_CONFIRM;
            drawConfirmScreen();
            return;
        }
    }
    else if (key == '0' && 9 < ALERT_COUNT) {
        selectedAlertIndex = 9;
        // INSTANT: Skip menu redraw, go straight to confirm
        previousScreen = SCREEN_MENU;
        currentScreen = SCREEN_CONFIRM;
        drawConfirmScreen();
        return;
    }
    // Navigation
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
    // Actions
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
    
    // FAST PARTIAL UPDATE: Only redraw if scroll changed, otherwise update selection only
    if (selectionChanged) {
        if (menuScrollOffset != oldScrollOffset) {
            // Scroll position changed - full redraw needed
            drawMenuScreen();
        } else {
            // Same scroll - just update the two affected items instantly
            updateMenuSelection(oldSelection, selectedAlertIndex);
        }
    }
}

void handleConfirmInput(char key) {
    if (key == '*') {
        // INSTANT SEND - Draw sending screen while transmitting
        currentScreen = SCREEN_SENDING;
        drawSendingScreen();
        
        // Transmit immediately (no delay)
        lastTransmitSuccess = transmitAlert();
        
        retryCount = 0;
        currentScreen = SCREEN_RESULT;
        drawResultScreen();  // Instant transition to result
    }
    else if (key == '#') {
        currentScreen = SCREEN_MENU;
        drawMenuScreen();  // Instant back to menu
    }
}

void handleResultInput(char key) {
    if (lastTransmitSuccess) {
        // Any key returns to menu instantly
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
                
                // Transmit immediately
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
    // Any key returns to menu instantly
    currentScreen = SCREEN_MENU;
    drawMenuScreen();
}

void handleUserManualInput(char key) {
    if (key == 'A' && manualPage > 0) {
        manualPage--;
        drawUserManualScreen();  // Instant page flip
    }
    else if (key == 'B' && manualPage < MANUAL_TOTAL_PAGES - 1) {
        manualPage++;
        drawUserManualScreen();  // Instant page flip
    }
    else if (key == '#' || key == '*') {
        currentScreen = SCREEN_MENU;
        drawMenuScreen();  // Instant back to menu
    }
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              SETUP
// ═══════════════════════════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
    
    #if SERIAL_DEBUG_ENABLED
    printDebugHeader();
    #endif
    
    Serial.println(F("[INIT] Starting LifeLine TX..."));
    
    // GPIO
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    clearAllLEDs();
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(TFT_CS, OUTPUT);
    pinMode(LORA_CS, OUTPUT);
    pinMode(LORA_RST, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    digitalWrite(LORA_CS, HIGH);
    
    // Manual LoRa reset for reliable initialization
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);
    
    // SPI
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    
    // LoRa - WORKING CONFIGURATION
    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
    if (LoRa.begin(LORA_FREQUENCY)) {
        LoRa.setSpreadingFactor(LORA_SF);
        LoRa.setSignalBandwidth(LORA_BW);
        LoRa.enableCrc();
        loraInitialized = true;
        Serial.printf("[INIT] LoRa OK @ %.1f MHz, SF%d, BW125kHz, CRC enabled\\n", LORA_FREQUENCY / 1E6, LORA_SF);
    } else {
        loraInitialized = false;
        Serial.println(F("[INIT] LoRa FAILED!"));
    }
    
    // TFT
    tft.init(NATIVE_WIDTH, NATIVE_HEIGHT);
    tft.setRotation(SCREEN_ROTATION);
    tft.fillScreen(COLOR_BG_PRIMARY);
    Serial.printf("[INIT] TFT: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Boot screen
    currentScreen = SCREEN_BOOT;
    drawBootScreen();
    
    // LED flash
    setLED(LED_GREEN, true);
    setLED(LED_RED, true);
    // delay(200); // Removed for instant transition
    clearAllLEDs();
    
    Serial.println(F("[INIT] Ready"));
    Serial.printf("[INIT] Device: TX #%03d\n", DEVICE_ID);
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              MAIN LOOP
// ═══════════════════════════════════════════════════════════════════════════════════

void loop() {
    switch (currentScreen) {
        case SCREEN_BOOT:
            if (millis() - bootStartTime >= BOOT_DISPLAY_TIME) {
                currentScreen = SCREEN_MENU;
                drawMenuScreen();
                Serial.println(F("[STATE] -> MENU"));
            }
            break;
            
        case SCREEN_MENU:
        case SCREEN_CONFIRM:
        case SCREEN_SYSTEM_INFO:
        case SCREEN_USER_MANUAL:
            {
                char key = keypad.getKey();
                #if SERIAL_DEBUG_ENABLED
                if (!key) {
                    key = readSerialKey();
                    if (key) Serial.printf("[SERIAL] Key: %c\n", key);
                }
                #endif
                if (key) handleKeyPress(key);
            }
            break;
            
        case SCREEN_SENDING:
            break;
            
        case SCREEN_RESULT:
            {
                if (lastTransmitSuccess && RESULT_SUCCESS_TIME > 0) {
                    if (millis() - resultStartTime >= RESULT_SUCCESS_TIME) {
                        clearAllLEDs();
                        currentScreen = SCREEN_MENU;
                        drawMenuScreen();
                        Serial.println(F("[STATE] Auto -> MENU"));
                    }
                }
                
                char key = keypad.getKey();
                #if SERIAL_DEBUG_ENABLED
                if (!key) {
                    key = readSerialKey();
                    if (key) Serial.printf("[SERIAL] Key: %c\n", key);
                }
                #endif
                if (key) handleKeyPress(key);
            }
            break;
    }
    
    delay(10);
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                              END OF FILE
// ═══════════════════════════════════════════════════════════════════════════════════
