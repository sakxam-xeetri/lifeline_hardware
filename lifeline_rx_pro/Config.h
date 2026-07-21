#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

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
#define API_ENDPOINT        "https://zenithkandel.com.np/lifeline/API/Create/message.php"
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

// 16x2 LCD Display Pins (I2C)
#define LCD_SDA     4       // I2C Data
#define LCD_SCL     16      // I2C Clock
#define LCD_ADDR    0x27    // Default I2C Address for 1602 LCD (PCF8574)
#define LCD_COLS    16
#define LCD_ROWS    2

// LED Indicator Pins
#define LED_WIFI    21      // WiFi connection status indicator (active HIGH when connected)
#define LED_DATA    13      // Data receive blink indicator (active HIGH on packet RX)
#define LED_GREEN   LED_WIFI
#define LED_RED     LED_DATA

// Audio Feedback
#define BUZZER_PIN  12      // Buzzer

// Dedicated WiFi Portal Push Button
#define WIFI_PORTAL_PIN  14 // Push button on GPIO 14 (Active LOW with internal pullup)

// ═══════════════════════════════════════════════════════════════════════════════════
//                              TIMING CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════════════
#define BOOT_DISPLAY_TIME       2000    // Boot screen duration (ms) - ~2 seconds
#define ALERT_DISPLAY_TIME      15000   // Alert display time before auto-return (ms) - 15 seconds
#define NO_WIFI_TIMEOUT         60000   // Timeout on No WiFi screen (ms) - 60 seconds
#define WIFI_SPLASH_TIME        1500    // Duration of WiFi connected splash screen (ms)
#define IDLE_PULSE_INTERVAL     600     // Pulse animation interval (ms)
#define HISTORY_MAX_ITEMS       10      // Maximum alerts in history
#define LONG_PRESS_DURATION     3000    // 3 seconds hold required on WiFi button
#define RX_BLINK_DURATION_MS    250     // LED ON time in milliseconds when data packet is received

// ═══════════════════════════════════════════════════════════════════════════════════
//                              LCD CUSTOM CHARACTERS (CGRAM 0-7)
// ═══════════════════════════════════════════════════════════════════════════════════
const uint8_t glyphRadar[8] = {
    0b00000, 0b01110, 0b10001, 0b00100, 0b01010, 0b00000, 0b00100, 0b00000
};

const uint8_t glyphBell[8] = {
    0b00100, 0b01110, 0b01110, 0b01110, 0b11111, 0b00000, 0b00100, 0b00000
};

const uint8_t glyphCheck[8] = {
    0b00000, 0b00001, 0b00011, 0b10110, 0b11100, 0b01000, 0b00000, 0b00000
};

const uint8_t glyphSignal[8] = {
    0b00001, 0b00001, 0b00101, 0b00101, 0b10101, 0b10101, 0b00000, 0b00000
};

const uint8_t glyphWarn[8] = {
    0b00100, 0b01110, 0b11111, 0b11011, 0b11011, 0b11111, 0b00100, 0b00000
};

// ═══════════════════════════════════════════════════════════════════════════════════
//                              ALERT DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════════════
#define ALERT_COUNT 15

const char* const alertNames[ALERT_COUNT] = {
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

const char* const alertNamesShort[ALERT_COUNT] = {
    "EMERGENCY",   "MEDICAL EMERG", "MED SHORTAGE", "EVACUATION",
    "STATUS OK",   "INJURY",        "FOOD SHORT",   "WATER SHORT",
    "WEATHER",     "LOST PERSON",   "ANIMAL ATTK",  "LANDSLIDE",
    "SNOW STORM",  "EQUIP FAIL",    "OTHER"
};

const uint8_t alertPriority[ALERT_COUNT] = {
    0, 0, 2, 0, 3, 1, 2, 2, 2, 1, 1, 1, 1, 2, 4
};

const char* const priorityLabels[] = {"CRIT", "HIGH", "MED ", "OK  ", "INFO"};
const char* const priorityLabelsShort[] = {"CR", "HI", "ME", "OK", "IN"};

// ═══════════════════════════════════════════════════════════════════════════════════
//                              DATA STRUCTURES & STATE ENUMS
// ═══════════════════════════════════════════════════════════════════════════════════
struct WiFiNetwork {
    String ssid;
    String password;
};

struct AlertRecord {
    int deviceId;
    int alertIndex;
    int rssi;
    unsigned long timestamp;
};

enum ScreenState {
    SCREEN_BOOT,            // 0 - Boot screen (~2s animation)
    SCREEN_WIFI_SPLASH,     // 1 - WiFi connected splash screen (~1.5s)
    SCREEN_NO_WIFI,         // 2 - No WiFi internet prompt screen
    SCREEN_COUNTDOWN,       // 3 - WiFi button hold progress bar countdown
    SCREEN_PORTAL,          // 4 - WiFi setup AP portal active screen
    SCREEN_IDLE,            // 5 - Listening / home screen
    SCREEN_ALERT,           // 6 - Alert display screen (15s timeout)
    SCREEN_HISTORY,         // 7 - History
    SCREEN_SYSTEM_INFO      // 8 - System info
};

#define SERIAL_DEBUG_ENABLED true
#define SERIAL_BAUD_RATE     115200

#endif // CONFIG_H
