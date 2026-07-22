# LifeLine Dual-ESP32 Sensor Node — Working Principle & Operating Architecture

This document details the operational mechanics, signal processing pipeline, sensor fusion algorithms, health scoring equations, and inter-MCU communication protocols powering the **LifeLine Dual-ESP32 Sensor Node**.

---

## 1. End-to-End System Operation Workflow

```text
+-----------------------------------------------------------------------------------+
|                        ESP32 #1: SENSOR PROCESSING UNIT (SPU)                     |
+-----------------------------------------------------------------------------------+
| 1. High-Frequency Sampling Loop (5 Hz): Reads GPS, DHT, MPU6050, MQ135, ADC      |
| 2. Digital Signal Processing: Moving Average Windowing (10 samples) on Accelerometer|
| 3. Risk & Health Computation: Calculate Node Health (0-100%) & Risk Score (0-100%) |
| 4. Sensor Fusion Engine: Evaluate Fire, Earthquake, Landslide, Gas & Heat Rules   |
| 5. Hardware SOS Detection: Instant GPIO 26 Interrupt check                         |
| 6. Binary Framing: Pack 48-byte struct with CRC-16-CCITT checksum                  |
| 7. Transmit to CCU over UART2 (115200 Baud)                                      |
+------------------------------------------+----------------------------------------+
                                           |
                              COBS/CRC16 UART Stream
                                           |
+------------------------------------------v----------------------------------------+
|                   ESP32 #2: COMMUNICATION CONTROLLER UNIT (CCU)                   |
+-----------------------------------------------------------------------------------+
| 1. Non-Blocking Ring Buffer Parser: Collect stream & check Magic Bytes ('L', 'F') |
| 2. CRC Verification: Validate 16-bit checksum against computed payload hash       |
| 3. Payload Conversion: Format into Base Station payload format ("TX003,F")        |
| 4. Optional AES-128 Stream Cipher: Obfuscate packet if security flag is active   |
| 5. LoRa SX1278 RF Transmission: 433 MHz, SF12, BW 125 kHz, 18 dBm                 |
| 6. Exponential Backoff Retries: Retry up to 3 times on RF collision/failure      |
+------------------------------------------+----------------------------------------+
                                           |
                                     LoRa 433 MHz RF
                                           |
+------------------------------------------v----------------------------------------+
|                     BASE STATION RECEIVER (`lifeline_rx_pro`)                    |
+-----------------------------------------------------------------------------------+
```

---

## 2. Sensor Acquisition & DSP Filtering Algorithms

### A. Accelerometer Moving Average & Vector Magnitude
To eliminate false alarms caused by temporary mechanical bumps or vibration noise, the MPU6050 accelerometer reads raw 3-axis acceleration vectors ($a_x, a_y, a_z$) in units of $g$.

1. **Vector Magnitude Computation**:
   $$A_{\text{raw}} = \sqrt{a_x^2 + a_y^2 + a_z^2}$$

2. **10-Sample Circular Moving Average**:
   $$\bar{A}[k] = \frac{1}{N} \sum_{i=0}^{N-1} A_{\text{raw}}[k-i], \quad \text{where } N = 10$$

3. **Tilt Angle Calculation**:
   $$\theta_{\text{tilt}} = \arccos\left(\frac{a_z}{A_{\text{raw}}}\right) \times \left(\frac{180}{\pi}\right)$$

---

## 3. Sensor Fusion & Emergency Classification Logic

Instead of transmitting raw isolated values, the SPU runs real-time sensor fusion across multiple sensor domains:

```text
                    +--------------------+
                    |  MQ135 Gas > 600   |
                    +---------+----------+
                              |
                              +-----------------> [ FIRE ALERT: Priority 5 ]
                              |                   (Forest Fire / Explosion)
                    +---------+----------+
                    | Temp >= 42.0 deg C |
                    +--------------------+

                    +--------------------+
                    | MPU Vibe > 1.8g    |
                    | (>= 15 samples)    | ---------> [ EARTHQUAKE: Priority 5 ]
                    +--------------------+

                    +--------------------+
                    | Tilt > 60 deg AND  |
                    | Impact > 3.0g      | ---------> [ LANDSLIDE: Priority 4 ]
                    +--------------------+

                    +--------------------+
                    | SOS Button Pressed | ---------> [ MANUAL SOS: Priority 5 ]
                    +--------------------+
```

### Fusion Rules Matrix
- **Forest Fire (`'F'`)**: Triggered when **Gas PPM $>600$** AND **Temperature $\ge 42.0^\circ\text{C}$**.
- **Earthquake (`'Q'`)**: Triggered when acceleration magnitude anomaly $|A_{\text{raw}} - 1.0g| > 0.8g$ is sustained for at least 15 consecutive sample cycles ($3\text{ seconds}$).
- **Landslide (`'L'`)**: Triggered when tilt angle $\theta_{\text{tilt}} > 60^\circ$ combined with a high-impact acceleration spike ($A_{\text{raw}} > 3.0g$).
- **Gas Leak (`'G'`)**: Triggered when MQ135 PPM $> 600$ without temperature spike.
- **Heat Wave (`'H'`)**: Triggered when temperature $T \ge 45.0^\circ\text{C}$.
- **Cold Wave (`'C'`)**: Triggered when temperature $T \le 0.0^\circ\text{C}$.
- **High Humidity (`'W'`)**: Triggered when humidity $H \ge 85\%$.
- **Low Battery (`'B'`)**: Triggered when battery voltage $V_{\text{batt}} \le 3.3\text{V}$ ($\le 15\%$).
- **Manual SOS (`'S'`)**: Instant hardware interrupt triggered by pushing the physical SOS button on GPIO 26.

---

## 4. Quantitative Health & Risk Scoring Equations

### A. Node Health Score ($H_{\text{node}} \in [0, 100]\%$)
Measures the operational integrity of the hardware device:
$$H_{\text{node}} = 100 - P_{\text{ENV}} - P_{\text{GPS}} - P_{\text{BAT}}$$
- $P_{\text{ENV}} = 15$ if DHT sensor read fails, else $0$.
- $P_{\text{GPS}} = 10$ if GPS fix is invalid, else $0$.
- $P_{\text{BAT}} = \max\left(0, \frac{50 - \text{Battery}\%}{2}\right)$ when battery drops below $50\%$.

---

### B. Environmental Risk Score ($R_{\text{env}} \in [0, 100]\%$)
Quantifies overall environmental hazard level in the vicinity of the node:
$$R_{\text{env}} = \min\left(100, R_{\text{gas}} + R_{\text{temp}} + R_{\text{seismic}}\right)$$
- **Gas Contribution**: $R_{\text{gas}} = \min\left(40, \frac{\text{PPM} - 100}{15}\right)$ for $\text{PPM} > 100$.
- **Temperature Contribution**: $R_{\text{temp}} = \begin{cases} (T - 35) \times 2.5 & \text{if } T > 35^\circ\text{C} \\ (5 - T) \times 2.0 & \text{if } T < 5^\circ\text{C} \\ 0 & \text{otherwise} \end{cases}$
- **Seismic/Motion Contribution**: $R_{\text{seismic}} = 30$ if Earthquake, $20$ if Landslide/Impact, $5$ if general motion.

---

## 5. Inter-MCU UART Packet Protocol & CRC-16 Checksum

To guarantee zero packet corruption between SPU and CCU, data is serialized into a fixed 48-byte binary structure:

```cpp
#pragma pack(push, 1)
typedef struct {
    uint8_t  magic1;          // 'L'
    uint8_t  magic2;          // 'F'
    uint8_t  version;         // 0x01
    uint16_t node_id;         // Unique Transmitter Node ID (e.g. 3)
    uint8_t  emergency_code;  // ASCII Emergency Code ('F', 'Q', 'S', 'N', etc.)
    uint8_t  priority;        // Priority Level (1 to 5)
    uint8_t  health_score;    // 0 - 100%
    uint8_t  risk_score;      // 0 - 100%
    uint8_t  battery_percent; // 0 - 100%
    int32_t  lat_deg_e7;      // Latitude * 10,000,000
    int32_t  lon_deg_e7;      // Longitude * 10,000,000
    int16_t  alt_meters;      // Altitude in meters
    uint8_t  sat_count;       // Satellites in view
    uint8_t  gps_fix;         // 1 = Valid Fix, 0 = No Fix
    int16_t  temp_c_x10;     // Temperature * 10
    uint16_t humidity_x10;    // Humidity * 10
    uint16_t pressure_hpa;    // Pressure in hPa
    uint16_t gas_ppm;         // Gas PPM
    int16_t  accel_x_mg;      // Accel X (mG)
    int16_t  accel_y_mg;      // Accel Y (mG)
    int16_t  accel_z_mg;      // Accel Z (mG)
    int16_t  gyro_x_dps;      // Gyro X (deg/sec)
    int16_t  gyro_y_dps;      // Gyro Y (deg/sec)
    int16_t  gyro_z_dps;      // Gyro Z (deg/sec)
    uint16_t crc16;           // CRC-16-CCITT Checksum
} TelemetryPacket;
#pragma pack(pop)
```

### CRC-16 Polynomial
Polynomial: $X^{16} + X^{12} + X^5 + 1$ (`0x1021`, initial value `0xFFFF`). The CCU re-evaluates CRC-16 over the first 46 bytes and discards any corrupted frame before LoRa transmission.
