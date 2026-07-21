# LifeLine Emergency Hardware — Complete Feature Matrix

This document provides a comprehensive list of all technical, hardware, software, user interface, communication, and cloud features offered by the **LifeLine Emergency Hardware Platform** (consisting of the **LifeLine TX Pro Field Transmitter** and **LifeLine RX Pro Base Station Receiver**).

---

## 1. LifeLine TX Pro — Emergency Field Transmitter

The **LifeLine TX Pro** is a ruggedized, portable emergency communication transmitter designed for deployment in remote, off-grid regions (e.g., Himalayan villages, mountaineering routes, disaster relief zones).

### 📡 Communication & Wireless Hardware
* **LoRa SX1278 Transceiver (433 MHz)**: Long-range spread-spectrum RF communication capable of penetrating mountains, foliage, and harsh terrain.
* **Maximum Sensitivity RF Parameters**:
  * **Spreading Factor**: SF12 (Maximum range & obstacle penetration)
  * **Bandwidth**: 125 kHz
  * **Coding Rate**: 4/8 (Maximum error correction for noisy environments)
  * **Preamble Length**: 16 symbols
  * **Transmit Power**: 18 dBm (~63 mW boosted RF output)
  * **Sync Word**: `0x12` (Isolated emergency channel network)
* **Compact Packet Protocol**: Structured, lightweight payload standard: `TX[ID],[ALERT_CODE]` (e.g., `TX003,A`).

### 📟 Display & Visual User Interface
* **2.8" SPI Color TFT Display (ST7789)**:
  * **Adaptive Resolution Engine**: Supports both 240×320 and 320×480 screen configurations in landscape mode.
  * **High-Contrast Dark Theme**: Ultra-modern dark palette (RGB565 `#0D0D0F` background, neon green `#00FF87`, electric red `#FF3B3B`, cyan `#00D4FF`) designed for anti-glare sunlight readability.
* **Interactive Graphical Interface**:
  * **Animated Boot Screen**: Displays radar scanning graphic, system hardware check, and firmware version.
  * **Category-Based Alert Navigation**: Organized menus for Emergency, Medical, Utility, Status, and System Info.
  * **15 Pre-configured Emergency Alert Types**:
    1. `EMERGENCY` (Critical)
    2. `MEDICAL EMERGENCY` (Critical)
    3. `MEDICINE SHORTAGE` (Medium)
    4. `EVACUATION NEEDED` (Critical)
    5. `STATUS OK` (Info)
    6. `INJURY REPORTED` (High)
    7. `FOOD SHORTAGE` (Medium)
    8. `WATER SHORTAGE` (Medium)
    9. `WEATHER ALERT` (Medium)
    10. `LOST PERSON` (High)
    11. `ANIMAL ATTACK` (High)
    12. `LANDSLIDE` (High)
    13. `SNOW STORM` (High)
    14. `EQUIPMENT FAILURE` (Medium)
    15. `OTHER EMERGENCY` (Neutral)
  * **Confirmation & Hold-to-Confirm Dialog**: Safety screens to prevent accidental SOS broadcasts.
  * **Live Transmission Screen**: Displays RF frequency, TX power, payload code, and animated signal waves.
  * **Transmission Result Screen**: Instant visual feedback (Green box for sent success / Red alert box for failure).
  * **System Information Screen**: Live telemetry displaying Device ID, Firmware Version, LoRa RF status, and power stats.

### ⌨️ Controls & Audio/Visual Feedback
* **4×4 Matrix Keypad Integration**: Tactile numeric/alphanumeric keypad for direct navigation in extreme cold (usable with gloves).
* **Dual LED Indicators**:
  * **Green LED**: Successful packet transmission & system ready.
  * **Red LED**: Error / transmission failure alert.
* **Piezo Audio Buzzer**: Key click sound effect, confirm chime, and failure warning alarm.

---

## 2. LifeLine RX Pro — Emergency Base Station Receiver

The **LifeLine RX Pro** is a high-reliability base station receiver installed in emergency command centers, police stations, hospitals, or local administration offices.

### 📡 Wireless & Reception Core
* **LoRa SX1278 Receiver (433 MHz)**: Continuously listens for emergency transmissions from field units.
* **Real-time Signal Analysis**: Calculates Signal Strength (RSSI in dBm) for every incoming emergency packet to estimate sender distance and link quality.

### 📟 Display & Audio Interface
* **16×2 Character I2C LCD Display (PCF8574 @ 0x27)**:
  * Low-power industrial character display with dynamic backlight.
  * **Custom CGRAM Character Glyphs**: Custom-designed 5x8 pixel icons for Radar (`CGRAM 0`), Bell (`CGRAM 1`), Checkmark (`CGRAM 2`), Signal Bars (`CGRAM 3`), and Warning (`CGRAM 4`).
* **Multi-Screen State Machine**:
  * `SCREEN_BOOT`: Boot animation and hardware initialization.
  * `SCREEN_IDLE`: Listening screen showing pulse animation, active Wi-Fi network, NTP clock time, and total alerts received.
  * `SCREEN_ALERT`: Instant high-priority alert popup displaying Device ID, Alert Name, Priority Label (`CRIT`, `HIGH`, `MED`, `OK`), RSSI, and timestamp.
  * `SCREEN_NO_WIFI`: Standalone fallback display when Wi-Fi is unavailable (with 60s auto-timeout back to local monitoring).
  * `SCREEN_COUNTDOWN`: Visual progress bar when holding the Wi-Fi setup button.
  * `SCREEN_PORTAL`: Displays setup Access Point name (`LifeLine-RX-Setup`) and IP address (`192.168.4.1`).
  * `SCREEN_OTA`: Dedicated remote update screen displaying live download percentage and progress bar.
* **Audible Siren & LED Feedback**:
  * **Red Data LED**: Blinks instantly on LoRa packet arrival.
  * **Green Wi-Fi LED**: Continuous status indication for network connectivity.
  * **Loud Siren Buzzer**: Plays distinctive pitch emergency alarm when a critical alert arrives.

### 🌐 Connectivity, Web Portal & Cloud Integration
* **Multi-Wi-Fi Auto-Connect**:
  * Stores up to 3 Wi-Fi network profiles in ESP32 Flash memory (`Preferences`).
  * Automatically scans and connects to the strongest saved network on startup.
* **Captive Portal Wi-Fi Manager (`WiFiPortal`)**:
  * Dedicated physical hardware push button (GPIO 14).
  * Hold for 3 seconds (with LCD progress bar countdown) to launch local AP `LifeLine-RX-Setup` at `192.168.4.1`.
  * Mobile-friendly web dashboard to scan nearby Wi-Fi SSIDs, enter passwords, and save network profiles.
* **Cloud REST API Gateway (`APIClient`)**:
  * Pushes incoming emergency alerts directly to a central cloud server via HTTPS POST.
  * Payload includes `deviceId`, `alertIndex`, `rssi`, `receiverId`, and timestamp.
* **NTP Network Time Synchronization**: Syncs with global NTP servers (`pool.ntp.org`, `asia.pool.ntp.org`) with configurable time zone offsets (e.g. Nepal Standard Time GMT+5:45).

### 🚀 Remote HTTPS Over-The-Air (OTA) System (`OTAManager`)
* **Global Cloud Firmware Updates**:
  * Checks your VPS website (`version.json`) over HTTPS automatically on every boot.
  * Sends device telemetry headers (`X-ESP32-MAC`, `X-ESP32-Firmware`, `X-ESP32-FreeHeap`) to your server.
  * Compares semantic versions (`isVersionNewer`).
* **Live LCD Progress Feedback**:
  * Renders real-time download percentage and 16-character progress bar (`Updating FW  45%`) on the receiver screen.
* **Dual-Partition Crash Protection**:
  * Downloads firmware directly to ESP32 `ota_1` partition.
  * Automatic rollback safety: if flash fails or binary is corrupted, the chip safely reboots back to `ota_0`.

---

## 3. System Feature Comparison Matrix

| Feature | LifeLine TX Pro (Transmitter) | LifeLine RX Pro (Receiver) |
| :--- | :---: | :---: |
| **Role** | Emergency SOS Broadcast Unit | Central Command Base Station |
| **Microcontroller** | ESP32 | ESP32 |
| **RF Module** | SX1278 LoRa @ 433 MHz | SX1278 LoRa @ 433 MHz |
| **Display Hardware** | 2.8" Color TFT (ST7789) | 16x2 Character I2C LCD |
| **User Input** | 4x4 Matrix Keypad | Dedicated Wi-Fi Push Button |
| **Wi-Fi Connectivity** | N/A (Off-Grid RF Only) | Multi-Wi-Fi (Stores 3 SSIDs) |
| **Captive Web Portal** | N/A | Yes (`192.168.4.1` Setup Portal) |
| **Cloud API Push** | N/A | Yes (HTTPS REST API Gateway) |
| **NTP Real-Time Clock** | N/A | Yes (Automatic NTP Sync) |
| **Remote HTTPS OTA** | N/A | Yes (Automatic VPS Check on Boot) |
| **Audio Feedback** | Key Click & Chimes | Emergency Siren Alarm |
| **Alert Priority Handling** | Yes (15 Categorized Alerts) | Yes (15 Categorized Alerts + RSSI) |
| **History Buffer** | N/A | 10 Recent Alerts in Memory |
| **Power Source** | Portable Battery Pack / USB | Base Station DC / USB Power |

---

## 4. End-to-End Emergency Flow

```text
 ┌─────────────────────────┐                     ┌─────────────────────────┐
 │   LifeLine TX Pro       │                     │    LifeLine RX Pro      │
 │  (Mountaineer/Village)  │                     │    (Base Station)       │
 │                         │                     │                         │
 │ 1. User selects "LANDSLIDE"                   │                         │
 │ 2. Keypad confirmation  │   LoRa 433 MHz RF   │                         │
 │ 3. Transmits "TX003,L"  ├────────────────────►│ 1. Receives RF packet   │
 │                         │   (Up to 10+ km)    │ 2. Reads RSSI (-78 dBm)  │
 │ 4. TFT displays SUCCESS │                     │ 3. Sounds loud Siren    │
 └─────────────────────────┘                     │ 4. LCD displays alert   │
                                                 └────────────┬────────────┘
                                                              │
                                                        HTTPS REST API
                                                              │
                                                              ▼
                                                 ┌─────────────────────────┐
                                                 │ Central Cloud Web API   │
                                                 │ (Emergency Dashboard)   │
                                                 └─────────────────────────┘
```
