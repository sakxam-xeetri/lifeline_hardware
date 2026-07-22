#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

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
#define TFT_CS      16      // Chip Select (unique for TFT)
#define TFT_DC      4       // Data/Command
#define TFT_RST     2       // Reset

// LED Indicator Pins
#define LED_GREEN   21      // Success indicator (active HIGH)
#define LED_RED     13      // Failure indicator (active HIGH)

// Audio Feedback
#define BUZZER_PIN  12      // Buzzer

// ═══════════════════════════════════════════════════════════════════════════════════
//                          DISPLAY AUTO-ADAPTIVE CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════

#define DISPLAY_240x320   1    // Standard 2.4" / 2.8" TFT
#define DISPLAY_320x480   2    // Larger 3.5" TFT

#define DISPLAY_TYPE  DISPLAY_240x320

#if DISPLAY_TYPE == DISPLAY_240x320
    #define NATIVE_WIDTH    240
    #define NATIVE_HEIGHT   320
#else
    #define NATIVE_WIDTH    320
    #define NATIVE_HEIGHT   480
#endif

#define SCREEN_ROTATION     1   // 0=Portrait, 1=Landscape, 2=Portrait inverted, 3=Landscape inverted

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

#define HEADER_HEIGHT       42                         // Fixed header height
#define FOOTER_HEIGHT       32                         // Fixed footer height
#define CONTENT_START_Y     (HEADER_HEIGHT + 4)        // Content area start
#define CONTENT_HEIGHT      (SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 6)
#define MARGIN              10                         // Universal margin
#define MARGIN_SMALL        5                          // Small margin
#define BORDER_RADIUS       6                          // Rounded corner radius

#define MENU_ITEM_HEIGHT    34                         // Height per menu item
#define VISIBLE_MENU_ITEMS  5                          // Visible items in scroll list
#define MENU_START_Y        (HEADER_HEIGHT + 28)       // Menu list start position

// Text sizes (Adafruit GFX scale factors)
#define TEXT_XLARGE         4   // Extra large (boot logo)
#define TEXT_LARGE          3   // Large headings
#define TEXT_MEDIUM         2   // Standard text
#define TEXT_SMALL          1   // Small text / labels

// UI Effects
#define GLOW_INTENSITY      3   // Glow effect layers
#define SHADOW_OFFSET       2   // Card shadow offset
#define GRADIENT_STEPS      8   // Steps for gradient simulation

// ═══════════════════════════════════════════════════════════════════════════════════
//                    PREMIUM PROFESSIONAL COLOR PALETTE (RGB565)
// ═══════════════════════════════════════════════════════════════════════════════════

#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

#define COLOR_BG_PRIMARY        RGB565(13, 13, 15)
#define COLOR_BG_HEADER         RGB565(10, 25, 41)
#define COLOR_BG_HEADER_ALT     RGB565(7, 19, 24)
#define COLOR_BG_CARD           RGB565(26, 26, 46)
#define COLOR_BG_CARD_ACTIVE    RGB565(37, 37, 66)
#define COLOR_BG_INPUT          RGB565(45, 45, 68)

#define COLOR_GLASS_LIGHT       RGB565(255, 255, 255)
#define COLOR_GLASS_DARK        RGB565(20, 20, 30)

#define COLOR_RED               RGB565(255, 59, 59)
#define COLOR_RED_DARK          RGB565(180, 30, 30)
#define COLOR_RED_BRIGHT        RGB565(255, 100, 100)
#define COLOR_RED_GLOW          RGB565(255, 80, 80)

#define COLOR_GREEN             RGB565(0, 255, 135)
#define COLOR_GREEN_DARK        RGB565(0, 180, 95)
#define COLOR_GREEN_BRIGHT      RGB565(50, 255, 160)
#define COLOR_GREEN_GLOW        RGB565(30, 255, 145)

#define COLOR_AMBER             RGB565(255, 184, 0)
#define COLOR_AMBER_DARK        RGB565(200, 140, 0)
#define COLOR_AMBER_BRIGHT      RGB565(255, 210, 50)
#define COLOR_AMBER_GLOW        RGB565(255, 195, 30)

#define COLOR_CYAN              RGB565(0, 212, 255)
#define COLOR_CYAN_DARK         RGB565(0, 150, 200)
#define COLOR_CYAN_BRIGHT       RGB565(80, 230, 255)
#define COLOR_CYAN_GLOW         RGB565(40, 220, 255)

#define COLOR_ORANGE            RGB565(255, 123, 0)
#define COLOR_ORANGE_DARK       RGB565(200, 90, 0)
#define COLOR_ORANGE_BRIGHT     RGB565(255, 150, 50)

#define COLOR_PURPLE            RGB565(168, 85, 247)
#define COLOR_PURPLE_DARK       RGB565(120, 60, 180)

#define COLOR_TEXT_PRIMARY      RGB565(255, 255, 255)
#define COLOR_TEXT_SECONDARY    RGB565(163, 177, 198)
#define COLOR_TEXT_MUTED        RGB565(100, 116, 139)
#define COLOR_TEXT_DARK         RGB565(10, 10, 10)
#define COLOR_TEXT_DISABLED     RGB565(60, 70, 85)

#define COLOR_ACCENT_LINE       RGB565(40, 50, 70)
#define COLOR_ACCENT_BRIGHT     RGB565(60, 80, 120)

#define COLOR_BORDER            RGB565(50, 60, 85)
#define COLOR_BORDER_FOCUS      COLOR_CYAN
#define COLOR_BORDER_SUCCESS    COLOR_GREEN
#define COLOR_BORDER_ERROR      COLOR_RED

#define COLOR_STATUS_CRITICAL   COLOR_RED
#define COLOR_STATUS_HIGH       COLOR_ORANGE
#define COLOR_STATUS_MEDIUM     COLOR_AMBER
#define COLOR_STATUS_LOW        COLOR_GREEN
#define COLOR_STATUS_NEUTRAL    COLOR_TEXT_SECONDARY

#define COLOR_BADGE_BG          RGB565(45, 45, 75)

// ═══════════════════════════════════════════════════════════════════════════════════
//                              ALERT DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════════════

#define ALERT_COUNT 15

extern const char* alertNames[ALERT_COUNT];
extern const char* alertNamesShort[ALERT_COUNT];
extern const uint8_t alertPriority[ALERT_COUNT];

char getAlertCode(int index);

// ═══════════════════════════════════════════════════════════════════════════════════
//                              TIMING CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════════

#define BOOT_DISPLAY_TIME       1000    // Boot screen duration (ms)
#define SENDING_MIN_DISPLAY     400     // Minimum sending screen time (ms)
#define RESULT_SUCCESS_TIME     1800    // Success screen auto-return (ms)
#define RESULT_FAILURE_TIME     0       // Failure requires user input
#define DEBOUNCE_DELAY          30      // Keypress debounce (ms)
#define SCROLL_REPEAT_DELAY     80      // Auto-scroll repeat delay (ms)
#define MAX_RETRY_ATTEMPTS      3       // Maximum transmission retry attempts

// ═══════════════════════════════════════════════════════════════════════════════════
//                              SERIAL DEBUG CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════

#define SERIAL_DEBUG_ENABLED true
#define SERIAL_BAUD_RATE     115200

// ═══════════════════════════════════════════════════════════════════════════════════
//                              STATE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════════════

enum ScreenState {
    SCREEN_BOOT,            // 0 - Initial boot/splash screen
    SCREEN_MENU,            // 1 - Main alert selection menu
    SCREEN_CONFIRM,         // 2 - Alert confirmation dialog
    SCREEN_SENDING,         // 3 - Transmission in progress
    SCREEN_RESULT,          // 4 - Transmission result (success/failure)
    SCREEN_SYSTEM_INFO,     // 5 - System information display
    SCREEN_USER_MANUAL      // 6 - User manual / help screen
};

extern ScreenState currentScreen;
extern ScreenState previousScreen;

extern int selectedAlertIndex;
extern int menuScrollOffset;
extern int manualPage;
#define MANUAL_TOTAL_PAGES 4

extern unsigned long bootStartTime;
extern unsigned long resultStartTime;
extern unsigned long lastKeyPressTime;
extern unsigned long lastTransmitTime;

extern bool lastTransmitSuccess;
extern int retryCount;
extern int totalTransmissions;
extern int successfulTransmissions;

extern bool loraInitialized;
extern int batteryPercent;

#endif // CONFIG_H
