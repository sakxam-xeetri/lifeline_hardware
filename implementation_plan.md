# Production Dual-ESP32 Sensor Node Architecture Plan (`sensornode.md`)

This implementation plan details the complete redesign and production-grade implementation of the **LifeLine Sensor Node (`lifeline_tx_pro`)**. The architecture transitions from a legacy single-ESP32 transmitter to an industrial **Dual-ESP32 Architecture** consisting of a **Sensor Processing Unit (SPU)** and a **Communication Controller Unit (CCU)** connected via a high-reliability UART protocol, fully compatible with the existing **LifeLine Base Station (`lifeline_rx_pro`)** and Cloud Dashboard.

---

## User Review Required

> [!IMPORTANT]
> **Dual-ESP32 Hardware Partitioning**:
> The system requires two physical ESP32 boards communicating over UART (TX2/RX2). 
> - **ESP32 #1 (SPU)** handles sensor acquisition, DSP filtering, sensor fusion, risk analysis, emergency classification, and UART framing.
> - **ESP32 #2 (CCU)** handles UART checksum verification, packet queuing, LoRa SX1278 transmission, retry logic, power/sleep optimization, and Base Station compatibility.

> [!NOTE]
> **Base Station Protocol Compatibility**:
> To ensure zero breaking changes for existing Base Station installations (`lifeline_rx_pro`), the CCU will default to sending the legacy short packet payload (`TX[ID],[ALERT_CODE]`) for standard alarms, while supporting an extended structured binary/JSON telemetry payload when full environmental reporting is enabled.

---

## Open Questions

> [!NOTE]
> 1. **Sensors Availability**: Do you intend to test this hardware in a PlatformIO multi-environment setup (e.g. `lifeline_tx_spu` and `lifeline_tx_ccu` subprojects)?
> 2. **AES Encryption**: Should AES-128 payload encryption between CCU and Base Station be enabled by default in `Config.h` or kept optional via `#define ENABLE_LORA_ENCRYPTION 0`?

---

## Proposed Changes

### Component 1: Firmware Architecture & Directory Restructuring

Reorganize `lifeline_tx_pro` into two modular firmware projects under PlatformIO (or ESP-IDF compatible C++ structure):
- `lifeline_tx_spu/`: Firmware for ESP32 #1 (Sensor Processing Unit).
- `lifeline_tx_ccu/`: Firmware for ESP32 #2 (Communication Controller Unit).
- `shared/`: Shared packet definitions, binary structures, CRC16 utilities, and protocol specs.

---

### Component 2: ESP32 #1 — Sensor Processing Unit (SPU)

#### [NEW] [SPU Config & Main](file:///d:/lifeline_hardware/lifeline_tx_spu/include/ConfigSPU.h)
- Centralized configuration header for sensor pins, sampling rates, moving average window sizes, and emergency detection thresholds (Heat wave, Cold wave, High humidity, Gas PPM, Fire threshold, Earthquake vibration threshold, GPS update intervals).

#### [NEW] [Sensor Manager Core](file:///d:/lifeline_hardware/lifeline_tx_spu/src/sensor_manager.cpp)
- Non-blocking FreeRTOS task coordinator (`vTaskSensorLoop`) that samples all attached sensor peripherals on dedicated intervals without `delay()` calls.

#### [NEW] [GPS Module Manager](file:///d:/lifeline_hardware/lifeline_tx_spu/src/gps_manager.cpp)
- Interfaces with NEO-6M GPS over Hardware Serial. Parses NMEA sentences (`$GPGGA`, `$GPRMC`) to extract Latitude, Longitude, Altitude, Speed, UTC Timestamp, Satellite Count, and Fix Status.

#### [NEW] [Environmental Sensor Manager](file:///d:/lifeline_hardware/lifeline_tx_spu/src/environment_manager.cpp)
- Reads DHT11/DHT22/BME280 temperature, humidity, and barometric pressure. Implements sensor noise calibration and environmental anomaly detection (Extreme Heat >45°C, Cold <0°C, High Humidity >85%, Dry Air <20%).

#### [NEW] [MPU6050 Motion & Seismic Manager](file:///d:/lifeline_hardware/lifeline_tx_spu/src/mpu_manager.cpp)
- 6-Axis Accelerometer & Gyroscope processor over I2C.
- Features moving average windowing (10 samples), tilt angle calculation, free-fall detection ($<0.2g$), sudden impact detection ($>3.5g$), fall detection, and continuous multi-axis vibration integration for **Earthquake Classification**.

#### [NEW] [Gas & Air Quality Manager](file:///d:/lifeline_hardware/lifeline_tx_spu/src/gas_manager.cpp)
- MQ135 analog sampling with baseline resistance ($R_0$) auto-calibration. Computes gas concentration (PPM) for smoke, air pollution, and hazardous gas leaks.

#### [NEW] [Emergency & Sensor Fusion Engine](file:///d:/lifeline_hardware/lifeline_tx_spu/src/emergency_detector.cpp)
- Sensor fusion engine cross-correlating multi-sensor data:
  - **Forest Fire Alert**: Gas Concentration > Threshold AND Temperature > Threshold.
  - **Landslide Warning**: High Acceleration Anomaly AND Barometric/Tilt Shift.
  - **Earthquake Alert**: Sustained MPU vibration energy over consecutive sample windows.
  - **SOS Override**: Hardware interrupt button press trigger.
  - **Low Battery Warning**: ADC battery monitoring ($<3.3\text{V}$).

#### [NEW] [Health & Risk Calculator](file:///d:/lifeline_hardware/lifeline_tx_spu/src/health_calculator.cpp)
- Computes real-time quantitative metrics:
  - `Node Health Score` (0–100% based on sensor status & voltage).
  - `Environmental Risk Score` (0–100% computed from gas, temp, & vibration indices).
  - `Emergency Priority Level` (1 = Normal Telemetry, 2 = Warning, 3 = High, 4 = Critical Emergency, 5 = SOS Emergency).

#### [NEW] [UART Packet Framer](file:///d:/lifeline_hardware/lifeline_tx_spu/src/uart_manager.cpp)
- Serializes telemetry data, health/risk scores, and emergency codes into a binary packet formatted with COBS framing and CRC-16-CCITT integrity verification, sent over hardware UART to the CCU.

---

### Component 3: ESP32 #2 — Communication Controller Unit (CCU)

#### [NEW] [CCU Config & Main](file:///d:/lifeline_hardware/lifeline_tx_ccu/include/ConfigCCU.h)
- Configuration header for LoRa parameters (433 MHz, SF12, BW 125 kHz, CR 4/8, TX Power 18 dBm, Sync Word `0x12`), UART pins (RX2/TX2), and retry/ACK thresholds.

#### [NEW] [UART Packet Receiver](file:///d:/lifeline_hardware/lifeline_tx_ccu/src/uart_receiver.cpp)
- High-speed non-blocking UART ring buffer parser. Validates CRC-16 checksums and unpacks telemetry structures from the SPU.

#### [NEW] [LoRa Communication Engine](file:///d:/lifeline_hardware/lifeline_tx_ccu/src/lora_manager.cpp)
- Manages SX1278 Transceiver via SPI interface.
- Constructs Base Station-compatible emergency packets (`TX[ID],[ALERT_CODE]`) for fast emergency reception, as well as structured telemetry frames.
- Implements packet transmission queue, ACK waiting timer, exponential backoff retries (up to 3 attempts), and packet deduplication.

#### [NEW] [Power & Sleep Manager](file:///d:/lifeline_hardware/lifeline_tx_ccu/src/power_manager.cpp)
- Manages ESP32 deep sleep cycles between routine telemetry updates. Configures external GPIO wake-up pin connected to SPU UART interrupt line for instant emergency wakeups.

#### [NEW] [Diagnostics & Diagnostics Logger](file:///d:/lifeline_hardware/lifeline_tx_ccu/src/diagnostics.cpp)
- Tracks device uptime, heap memory fragmentation, packet success/retry ratios, and RF signal quality metrics.

---

## Verification Plan

### Automated Build & Unit Tests
1. **Compilation Check**: Run PlatformIO build commands for both SPU and CCU targets:
   ```powershell
   pio run -d d:\lifeline_hardware\lifeline_tx_spu
   pio run -d d:\lifeline_hardware\lifeline_tx_ccu
   ```
2. **CRC-16 Unit Verification**: Test binary packet packing/unpacking and CRC verification between mock UART buffers.

### Hardware & Protocol Verification
1. **UART Data Integrity Test**: Verify that telemetry packets sent from SPU UART2 are correctly parsed by CCU UART2 without packet corruption or dropped bytes.
2. **Emergency Sensor Fusion Validation**:
   - Inject high temperature + gas analog values $\rightarrow$ confirm automatic **Fire Alert** generation.
   - Trigger continuous MPU vibration $\rightarrow$ confirm automatic **Earthquake Alert** generation.
   - Press SOS button $\rightarrow$ confirm instant priority 5 emergency transmission.
3. **LoRa Interoperability Test**: Verify that CCU LoRa transmissions are successfully received and displayed by the existing **LifeLine RX Pro Base Station (`lifeline_rx_pro`)**.
