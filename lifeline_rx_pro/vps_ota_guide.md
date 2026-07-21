# LifeLine RX Pro — VPS Remote OTA Deployment Guide

This guide explains how the **Remote Over-The-Air (OTA) Firmware Update System** works for your LifeLine RX Pro receiver, and provides complete step-by-step instructions and code for setting up your VPS backend.

---

## 1. How the OTA System Works

```
 ┌──────────────────────┐         1. HTTPS GET /ota/version.json         ┌────────────────────────┐
 │                      ├───────────────────────────────────────────────►│                        │
 │  LifeLine RX (ESP32) │                                                │   VPS Web Server       │
 │                      │◄───────────────────────────────────────────────┤ (zenithkandel.com.np)   │
 └──────────┬───────────┘        2. Returns JSON Manifest Payload        └───────────┬────────────┘
            │                                                                        │
            │  3. Compares Server Version vs Current Device Version                  │
            │     (e.g., 3.2.0 > 3.1.0 PRO)                                          │
            ▼                                                                        │
 ┌──────────────────────┐         4. HTTPS GET /ota/firmware_v3.2.0.bin              │
 │  16x2 LCD Status     ├────────────────────────────────────────────────────────────┘
 │ "Updating FW  45%"   │
 └──────────┬───────────┘         5. Streams .bin payload directly to ota_1 partition
            │
            ▼
 ┌──────────────────────┐
 │ Automatic Reboot     │
 │ Running FW v3.2.0    │
 └──────────┬───────────┘
```

1. **Boot Trigger**: Immediately after Wi-Fi connects on boot, `checkAndPerformOTA()` is executed.
2. **Manifest Request**: The ESP32 sends an HTTPS GET request to `https://zenithkandel.com.np/lifeline/ota/version.json` with telemetry headers (`X-ESP32-MAC`, `X-ESP32-Firmware`, `X-ESP32-FreeHeap`).
3. **LCD Status Update**: The 16x2 LCD shows `[Checking OTA...]`.
4. **Version Check**: If the VPS version is higher than the current device version (`FIRMWARE_VERSION` in `Config.h`), the device notifies the user via LCD and audio beep.
5. **Stream & Flash**: The ESP32 streams the `.bin` file directly into its secondary app partition (`ota_1`) while displaying real-time progress (`Updating FW 45%`) on the 16x2 LCD.
6. **Reboot**: On completion, the LCD shows `OTA Complete! Rebooting...` and boots into the new firmware.

---

## 2. VPS Directory & File Setup

### A. Folder Structure on VPS

Create an `ota` directory in your website root on your VPS:

```text
/var/www/zenithkandel.com.np/html/lifeline/ota/
│
├── version.json                <-- Release manifest
├── firmware_v3.1.0.bin         <-- Initial base release
└── firmware_v3.2.0.bin         <-- Future new update binary
```

---

### B. Creating `version.json`

Create `/var/www/zenithkandel.com.np/html/lifeline/ota/version.json`:

```json
{
  "version": "3.2.0",
  "url": "https://zenithkandel.com.np/lifeline/ota/firmware_v3.2.0.bin",
  "mandatory": false,
  "changelog": "Added remote OTA updates and improved LoRa stability."
}
```

---

## 3. Optional: Dynamic PHP Telemetry & Update Handler

If you want your VPS to **log device MAC addresses, IP addresses, and check timestamps**, you can use a PHP script instead of a static `version.json`.

Create `/var/www/zenithkandel.com.np/html/lifeline/ota/version.php`:

```php
<?php
header('Content-Type: application/json');

// Log telemetry headers from ESP32
$mac = isset($_SERVER['HTTP_X_ESP32_MAC']) ? $_SERVER['HTTP_X_ESP32_MAC'] : 'UNKNOWN';
$firmware = isset($_SERVER['HTTP_X_ESP32_FIRMWARE']) ? $_SERVER['HTTP_X_ESP32_FIRMWARE'] : 'UNKNOWN';
$ip = $_SERVER['REMOTE_ADDR'];
$timestamp = date('Y-m-d H:i:s');

// Append to log file on VPS
$logLine = "[$timestamp] IP: $ip | MAC: $mac | Current FW: $firmware\n";
file_put_contents(__DIR__ . '/ota_requests.log', $logLine, FILE_APPEND);

// Latest release metadata
$response = [
    "version" => "3.2.0",
    "url" => "https://zenithkandel.com.np/lifeline/ota/firmware_v3.2.0.bin",
    "notes" => "Production OTA release v3.2.0"
];

echo json_encode($response, JSON_PRETTY_PRINT);
?>
```

---

## 4. How to Deploy a New Firmware Update (Step-by-Step)

When you make code changes and want to deploy a new version to all receivers:

### Step 1: Update Firmware Version in Code
In `Config.h`, change the version macro:
```cpp
#define FIRMWARE_VERSION    "v3.2.0 PRO"   // Update version number
```

### Step 2: Build the Firmware Binary in PlatformIO
Open terminal in VSCode and run:
```bash
pio run
```
PlatformIO will compile your code and output the compiled binary file at:
```text
d:\lifeline_hardware\lifeline_rx_pro\.pio\build\esp32dev\firmware.bin
```

### Step 3: Rename and Upload to VPS
1. Rename `firmware.bin` to `firmware_v3.2.0.bin`.
2. Upload `firmware_v3.2.0.bin` to `/var/www/zenithkandel.com.np/html/lifeline/ota/` via SFTP/FTP/SCP:
   ```bash
   scp .pio/build/esp32dev/firmware.bin user@your-vps-ip:/var/www/zenithkandel.com.np/html/lifeline/ota/firmware_v3.2.0.bin
   ```

### Step 4: Update `version.json` on VPS
Update `version.json` to point to `3.2.0`:
```json
{
  "version": "3.2.0",
  "url": "https://zenithkandel.com.np/lifeline/ota/firmware_v3.2.0.bin"
}
```

---

## 5. Security & Fallback Features

1. **Automatic Dual-Partition Rollback**:
   The ESP32 uses dual partitions (`ota_0` and `ota_1`). If an update fails mid-download or the binary is corrupted, the ESP32 automatically cancels the update and reboots safely into the previous working firmware.
2. **HTTPS TLS Encryption**:
   All communication uses HTTPS over TLS via `WiFiClientSecure`.
3. **LCD Status Visibility**:
   The user sees every step of the update on the 16x2 LCD display, eliminating any ambiguity about device status.
