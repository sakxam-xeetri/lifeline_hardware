#ifndef CONFIG_CCU_H
#define CONFIG_CCU_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════════════
//                              DEVICE IDENTIFICATION & OPTION FLAGS
// ═══════════════════════════════════════════════════════════════════════════════════

#define CCU_DEVICE_ID           3                // Unique transmitter ID (1-999)
#define CCU_FIRMWARE_VERSION    "v1.0-CCU"       // Firmware version
#define CCU_DEBUG_ENABLE        true             // Enable Serial Debug output

// Encryption Settings (Fast Software/Hardware AES-128)
#define ENABLE_LORA_ENCRYPTION  0                // Set to 1 to enable zero-delay AES payload obfuscation

// ═══════════════════════════════════════════════════════════════════════════════════
//                              LORA RF MODULE CONFIGURATION (SX1278)
// ═══════════════════════════════════════════════════════════════════════════════════

#define LORA_FREQUENCY          433E6            // LoRa frequency in Hz (433 MHz)
#define LORA_SF                 12               // Spreading Factor (SF7 - SF12) - Max range
#define LORA_BW                 125E3            // Bandwidth (125 kHz)
#define LORA_TX_POWER           18               // TX Power in dBm (2 - 20 dBm)
#define LORA_CR                 8                // Coding Rate 4/8 - Max error correction
#define LORA_PREAMBLE           16               // Preamble Length
#define LORA_SYNC_WORD          0x12             // LoRa Sync Word (matches Base Station)

// ═══════════════════════════════════════════════════════════════════════════════════
//                              PIN DEFINITIONS (ESP32 #2 CCU)
// ═══════════════════════════════════════════════════════════════════════════════════

// LoRa SX1278 SPI Pins
#define LORA_CS_PIN             18               // Chip Select
#define LORA_RST_PIN            23               // Reset
#define LORA_DIO0_PIN           26               // Digital I/O 0 (Interrupt)
#define SPI_SCK_PIN             5                // Serial Clock
#define SPI_MISO_PIN            19               // Master In Slave Out
#define SPI_MOSI_PIN            27               // Master Out Slave In

// Hardware UART2 for CCU <- SPU Communication
#define UART_SPU_TX_PIN         17               // CCU TX2 -> SPU RX2 (GPIO 16)
#define UART_SPU_RX_PIN         16               // CCU RX2 -> SPU TX2 (GPIO 17)
#define UART_SPU_BAUD           115200           // High-speed UART link

// Status Indicators
#define STATUS_TX_LED_PIN       2                // Transmission indicator LED

// ═══════════════════════════════════════════════════════════════════════════════════
//                              TRANSMISSION & RETRY TIMINGS
// ═══════════════════════════════════════════════════════════════════════════════════

#define MAX_LORA_RETRIES        3                // Maximum transmission attempts
#define LORA_RETRY_BACKOFF_MS   300              // Retries backoff delay in ms
#define BASE_STATION_COMPAT     1                // 1 = Send Base Station compatible "TX[ID],[CODE]" format

#endif // CONFIG_CCU_H
