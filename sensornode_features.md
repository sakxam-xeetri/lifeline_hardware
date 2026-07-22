# LifeLine Dual-ESP32 Sensor Node — Technical Feature Matrix

This document lists all hardware, software, environmental, security, communication, and power management features offered by the **LifeLine Dual-ESP32 Sensor Node**.

---

## 1. Hardware & System Architecture Features

* **Dual-Microcontroller Architecture**:
  * **Dedicated Sensor Processing Unit (ESP32 #1 SPU)**: Isolated 240 MHz dual-core CPU dedicated strictly to sensor sampling, filtering, and sensor fusion.
  * **Dedicated Communication Controller Unit (ESP32 #2 CCU)**: Isolated 240 MHz dual-core CPU dedicated strictly to UART packet handling, AES encryption, and SX1278 LoRa RF transmission.
* **Fault Isolation & High Availability**: A crash or lockup in RF communication never affects sensor acquisition or SOS emergency button monitoring.
* **Inter-MCU High-Speed UART Link**: Hardware Serial 2 @ 115200 Baud with COBS byte framing and CRC-16-CCITT integrity verification.

---

## 2. Sensor Processing & Environmental Sensing Features

* **NEO-6M GPS Navigation Core**:
  * High-precision NMEA sentence parser extracting Latitude, Longitude, Altitude, Ground Speed, Satellite count, UTC Timestamp, and Fix Status.
  * Integer scaled $10^7$ coordinate encoding for low-bandwidth binary representation.
* **Environmental Monitoring (DHT11 / DHT22 / BME280)**:
  * Measures ambient Temperature ($\pm 0.5^\circ\text{C}$), Relative Humidity ($\pm 2\%$), and Atmospheric Pressure.
  * Automated threshold flags for Extreme Heat Wave ($>45^\circ\text{C}$), Extreme Cold Wave ($<0^\circ\text{C}$), High Humidity ($>85\%$), and Dry Air ($<20\%$).
* **6-DOF Inertial Motion Unit (MPU6050 Accelerometer & Gyroscope)**:
  * 10-sample circular moving average filter to suppress mechanical vibration noise.
  * Real-time vector magnitude calculation ($A = \sqrt{a_x^2 + a_y^2 + a_z^2}$).
  * Dynamic tilt angle tracking ($\theta_{\text{tilt}}$ relative to gravity).
  * Automated Free-Fall detection ($<0.3g$), Sudden Impact detection ($>3.0g$), Excessive Tilt warning ($>60^\circ$), and Seismic Vibration detection.
* **MQ135 Hazardous Gas & Air Quality Sensor**:
  * Baseline resistance ($R_0$) auto-calibration.
  * PPM estimation for smoke, hazardous gas leaks, and air pollution.

---

## 3. Sensor Fusion & Intelligent Emergency Detection Features

* **Multi-Sensor Cross-Correlation Engine**:
  * **Forest Fire / Explosion Alert**: Fuses MQ135 Gas PPM ($>600$) with Temperature ($\ge 42^\circ\text{C}$).
  * **Earthquake / Seismic Alert**: Fuses continuous multi-axis vibration ($>1.8g$) over sustained sample windows.
  * **Landslide / Structure Collapse**: Fuses tilt shift ($>60^\circ$) with sudden impact acceleration spikes.
* **Instant SOS Override**: Dedicated hardware interrupt push-button (GPIO 26) with 50 ms hardware/software debouncing for manual emergency SOS broadcasts.
* **Quantitative Health & Risk Scoring**:
  * **Node Health Score ($0 - 100\%$)**: Self-evaluates sensor connectivity, GPS lock status, and battery health.
  * **Environmental Risk Score ($0 - 100\%$)**: Real-time risk index aggregated across gas concentration, temperature anomalies, and seismic activity.

---

## 4. Communication, Security & Base Station Compatibility Features

* **SX1278 433 MHz LoRa Transceiver**:
  * Maximum penetration parameters: Spreading Factor SF12, Bandwidth 125 kHz, Coding Rate 4/8, Preamble Length 16, Transmit Power 18 dBm.
  * Sync Word `0x12` matching LifeLine Base Station receivers.
* **Base Station Protocol Compatibility**:
  * Transmits standard payload frames (`TX[ID],[ALERT_CODE]`, e.g. `TX003,F`) fully compatible with existing **LifeLine RX Pro Base Stations** and Cloud Dashboards.
* **Zero-Delay Fast AES Encryption Option**:
  * Optional 128-bit payload obfuscation/encryption stream cipher configurable via `#define ENABLE_LORA_ENCRYPTION 1`.
* **Automatic Retry & Exponential Backoff**:
  * Retries failed transmissions up to 3 times with dynamic delay backoff.

---

## 5. Power Management & Industrial Reliability Features

* **Li-Ion Battery ADC Voltage Monitoring**:
  * 1:2 precision resistor divider measuring battery voltage ($3.3\text{V} - 4.2\text{V}$) with percentage estimation.
* **Light Sleep Energy Conservation**:
  * Configurable light sleep states on CCU between telemetry dispatches.
* **FreeRTOS Non-Blocking Event Loops**:
  * Zero `delay()` calls in core sensor tasks to maintain sub-millisecond event responsiveness.
