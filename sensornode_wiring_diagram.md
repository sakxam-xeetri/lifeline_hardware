# LifeLine Dual-ESP32 Sensor Node — Circuit Wiring & Pinout Diagram

This document provides the complete physical wiring diagram, pinout matrices, power distribution scheme, and schematic breakdown for the **LifeLine Dual-ESP32 Sensor Node**.

---

## 1. System Block Diagram & Wiring Overview

```text
               +-------------------------------------------------------------+
               |                  POWER SUPPLY (Li-Ion / USB 5V)             |
               +------------------------------+------------------------------+
                                              |
                                     +--------+--------+
                                     |                 |
                                  +--v--+           +--v--+
                                  | 5V  |           | 5V  |
                               +--+-----+--+     +--+-----+--+
                               | ESP32 #1  |     | ESP32 #2  |
                               |   (SPU)   |     |   (CCU)   |
                               +--+-----+--+     +--+-----+--+
                                  | 3.3V            | 3.3V
           +----------------------+------+          |
           |          |          |       |          |
        +--v--+    +--v--+    +--v--+ +--v--+    +--v--+
        | GPS |    | DHT |    | MPU | | MQ  |    |LoRa |
        +-----+    +-----+    +-----+ +-----+    +-----+

                INTER-MCU LINK (HARDWARE UART2 @ 115200 BAUD)
               +--------------------------------------------+
               | ESP32 #1 TX2 (GPIO 17) ---> ESP32 #2 RX2 (GPIO 16) |
               | ESP32 #1 RX2 (GPIO 16) <--- ESP32 #2 TX2 (GPIO 17) |
               | Common Ground (GND)    <---> Common Ground (GND)    |
               +--------------------------------------------+
```

---

## 2. Pinout Connection Tables

### A. ESP32 #1 — Sensor Processing Unit (SPU)

| Peripheral Module | Sensor Pin | ESP32 #1 Pin | Power Rail | Notes / Signal Type |
| :--- | :--- | :--- | :--- | :--- |
| **Inter-MCU Link** | TX2 | **GPIO 17** | N/A | UART2 Transmit $\rightarrow$ CCU RX2 |
| | RX2 | **GPIO 16** | N/A | UART2 Receive $\leftarrow$ CCU TX2 |
| **NEO-6M GPS** | TX | **GPIO 15** | 3.3V / 5V | Hardware Serial 1 RX |
| | RX | **GPIO 4** | 3.3V / 5V | Hardware Serial 1 TX |
| **DHT11 / DHT22**| DATA | **GPIO 23** | 3.3V | 10k$\Omega$ pullup resistor to 3.3V |
| **MPU6050 IMU** | SDA | **GPIO 21** | 3.3V | I2C Data Line (4.7k$\Omega$ pullup) |
| | SCL | **GPIO 22** | 3.3V | I2C Clock Line (4.7k$\Omega$ pullup) |
| **MQ135 Gas** | AOUT | **GPIO 34** | 5V (VCC) / ADC34 | Analog Output (0V–3.3V scaled) |
| **Battery Monitor**| Divider | **GPIO 35** | ADC35 | Resistor Divider ($R_1 = 10\text{k}\Omega, R_2 = 10\text{k}\Omega$) |
| **SOS Button** | Pin 1 | **GPIO 26** | GND | Active LOW (Internal Pullup) |
| | Pin 2 | **GND** | GND | Common Ground |
| **Status LED** | Anode (+) | **GPIO 2** | 3.3V | Built-in / 220$\Omega$ resistor to GND |

---

### B. ESP32 #2 — Communication Controller Unit (CCU)

| Peripheral Module | Module Pin | ESP32 #2 Pin | Power Rail | Notes / Signal Type |
| :--- | :--- | :--- | :--- | :--- |
| **Inter-MCU Link** | RX2 | **GPIO 16** | N/A | UART2 Receive $\leftarrow$ SPU TX2 |
| | TX2 | **GPIO 17** | N/A | UART2 Transmit $\rightarrow$ SPU RX2 |
| **LoRa SX1278** | NSS / CS | **GPIO 18** | 3.3V | SPI Chip Select |
| | RESET | **GPIO 23** | 3.3V | Hardware Module Reset |
| | DIO0 | **GPIO 26** | 3.3V | Packet RX/TX Interrupt |
| | SCK | **GPIO 5** | 3.3V | SPI Serial Clock |
| | MISO | **GPIO 19** | 3.3V | SPI Master In Slave Out |
| | MOSI | **GPIO 27** | 3.3V | SPI Master Out Slave In |
| **TX Activity LED**| Anode (+) | **GPIO 2** | 3.3V | Blinks during RF transmission |

---

## 3. Power Distribution & Analog Conditioning

### A. Power Rail Wiring Scheme
```text
+5V / VIN Rail -------------------+-------------------+-------------------+
                                  |                   |                   |
                            [ESP32 #1 Vin]      [ESP32 #2 Vin]       [MQ135 VCC]
                                  |                   |
                               [3.3V Out]          [3.3V Out]
                                  |                   |
          +-----------------------+-------+           +---------+
          |           |           |       |                     |
      [GPS VCC]   [DHT VCC]   [MPU VCC] [Pullups]          [SX1278 VCC]

GND Rail -------------------------+-------------------+-------------------+ (COMMON GROUND)
```

> [!IMPORTANT]
> - **Common Ground**: ESP32 #1, ESP32 #2, all sensors, and the LoRa SX1278 module MUST share a solid common GND connection.
> - **LoRa 3.3V Supply**: The SX1278 module requires up to 120 mA during 18 dBm transmissions. Ensure the ESP32 3.3V regulator or an external 3.3V LDO (e.g. AMS1117-3.3) provides stable current with a 100$\mu$F decoupling capacitor across VCC and GND.

---

### B. Battery Voltage Divider Circuit
To measure a 3.7V Li-Ion battery (3.3V – 4.2V) safely using the ESP32 ADC (0V – 3.3V max):

```text
  Battery (+)  ---[ 10k Ohm Resistor (R1) ]---*---[ 10k Ohm Resistor (R2) ]--- Battery (-) / GND
                                               |
                                        ESP32 #1 GPIO 35 (ADC)
```
$$\text{V}_{\text{ADC}} = \text{V}_{\text{Battery}} \times \left(\frac{R_2}{R_1 + R_2}\right) = \frac{\text{V}_{\text{Battery}}}{2}$$
For a full battery at $4.2\text{V}$, $\text{V}_{\text{ADC}} = 2.1\text{V}$, which is safely within the $3.3\text{V}$ range of GPIO 35.

---

## 4. Hardware Assembly Checklist

1. **Common GND**: Verify zero-resistance continuity between ESP32 #1 GND, ESP32 #2 GND, and sensor GND pins.
2. **UART Cross-Wiring**:
   - SPU GPIO 17 (TX2) connected to CCU GPIO 16 (RX2).
   - SPU GPIO 16 (RX2) connected to CCU GPIO 17 (TX2).
3. **LoRa Antenna**: Ensure a 433 MHz helical or whip antenna is connected to the SX1278 module before powering on to avoid damaging the RF power amplifier.
4. **MQ135 Warm-Up**: Allow the MQ135 sensor to warm up for ~20 seconds after power-on to stabilize analog baseline readings.
