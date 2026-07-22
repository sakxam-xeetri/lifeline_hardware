# LifeLine Dual-ESP32 Sensor Node — Parallel Circuit Wiring & Pinout Diagram

This document provides the parallel physical pinout map, side-by-side header tables, and breadboard wiring guide for the **LifeLine Dual-ESP32 Sensor Node**.

---

## 1. Physical ESP32 DevKit Parallel Pin Header Map

```text
                     ESP32 30-PIN DEVKIT BOARD (TOP VIEW)
                             +-----------------+
                             |    USB PORT     |
                             +-----------------+
        [LEFT PIN HEADER]                          [RIGHT PIN HEADER]
       ------------------                          ------------------
      EN  (Reset Button)  [ 1]                 [30]  3V3 -------> Sensor 3.3V VCC
      VP  (GPIO 36 Analog)[ 2]                 [29]  GND -------> Common Ground
      VN  (GPIO 39 Analog)[ 3]                 [28]  GPIO 15 ---> NEO-6M GPS TX
  MQ135 Gas (Analog AO) ->[ 4] GPIO 34         [27]  GPIO 2  ---> Status LED
Battery ADC (Divider)  ->[ 5] GPIO 35         [26]  GPIO 4  ---> NEO-6M GPS RX
          (Reserved Pin)  [ 6] GPIO 32         [25]  GPIO 16 <--- CCU RX2 (UART)
          (Reserved Pin)  [ 7] GPIO 33         [24]  GPIO 17 ---> CCU TX2 (UART)
          (Reserved Pin)  [ 8] GPIO 25         [23]  GPIO 5  ---> SPI SCK (CCU)
  SOS Button (Active LOW)->[ 9] GPIO 26         [22]  GPIO 18 ---> SPI CS  (CCU)
     SPI MOSI (CCU LoRa) ->[10] GPIO 27         [21]  GPIO 19 ---> SPI MISO (CCU)
          (Reserved Pin)  [11] GPIO 14         [20]  GPIO 21 ---> MPU6050 I2C SDA
          (Reserved Pin)  [12] GPIO 12         [19]  GPIO 3  ---> USB RX0 (Debug)
          (Reserved Pin)  [13] GPIO 13         [18]  GPIO 1  ---> USB TX0 (Debug)
   Ground (Common GND) -> [14] GND             [17]  GPIO 22 ---> MPU6050 I2C SCL
  +5V Power (Vin Rail) -> [15] VIN             [16]  GPIO 23 ---> DHT11 / DHT22 Data
                             +-----------------+
```

---

## 2. Parallel Side-by-Side Pin Assignment Tables

### A. ESP32 #1 — Sensor Processing Unit (SPU)

| Physical Left Header Pin | Signal / Connected Module | Physical Right Header Pin | Signal / Connected Module |
| :--- | :--- | :--- | :--- |
| **VIN (Pin 15)** | **+5V Power Rail** | **3V3 (Pin 30)** | **+3.3V Sensor Power Rail** |
| **GND (Pin 14)** | **Common Ground** | **GND (Pin 29)** | **Common Ground** |
| **GPIO 34 (Pin 4)**| **MQ135 Gas Analog (AO)** | **GPIO 23 (Pin 16)**| **DHT11 / DHT22 Data** |
| **GPIO 35 (Pin 5)**| **Battery ADC (Divider)** | **GPIO 22 (Pin 17)**| **MPU6050 I2C SCL** |
| **GPIO 26 (Pin 9)**| **Hardware SOS Button** | **GPIO 21 (Pin 20)**| **MPU6050 I2C SDA** |
| *GPIO 32 (Pin 6)* | *Reserved for Expansion* | **GPIO 17 (Pin 24)**| **UART2 TX $\rightarrow$ CCU RX2** |
| *GPIO 33 (Pin 7)* | *Reserved for Expansion* | **GPIO 16 (Pin 25)**| **UART2 RX $\leftarrow$ CCU TX2** |
| *GPIO 25 (Pin 8)* | *Reserved for Expansion* | **GPIO 4 (Pin 26)** | **Hardware Serial 1 TX $\rightarrow$ GPS RX** |
| *GPIO 27 (Pin 10)*| *Reserved for Expansion* | **GPIO 2 (Pin 27)** | **Status LED Output** |
| *GPIO 14 (Pin 11)*| *Reserved for Expansion* | **GPIO 15 (Pin 28)**| **Hardware Serial 1 RX $\leftarrow$ GPS TX** |

---

### B. ESP32 #2 — Communication Controller Unit (CCU)

| Physical Left Header Pin | Signal / Connected Module | Physical Right Header Pin | Signal / Connected Module |
| :--- | :--- | :--- | :--- |
| **VIN (Pin 15)** | **+5V Power Rail** | **3V3 (Pin 30)** | **SX1278 3.3V VCC** |
| **GND (Pin 14)** | **Common Ground** | **GND (Pin 29)** | **Common Ground** |
| **GPIO 26 (Pin 9)**| **LoRa DIO0 Interrupt** | **GPIO 19 (Pin 21)**| **SX1278 SPI MISO** |
| **GPIO 27 (Pin 10)**| **SX1278 SPI MOSI** | **GPIO 18 (Pin 22)**| **SX1278 SPI CS / NSS** |
| *GPIO 34 (Pin 4)* | *Not Used on CCU* | **GPIO 5 (Pin 23)** | **SX1278 SPI SCK** |
| *GPIO 35 (Pin 5)* | *Not Used on CCU* | **GPIO 17 (Pin 24)**| **UART2 TX $\rightarrow$ SPU RX2** |
| *GPIO 23 (Pin 16)*| **SX1278 Hardware Reset** | **GPIO 16 (Pin 25)**| **UART2 RX $\leftarrow$ SPU TX2** |
| *GPIO 2 (Pin 27)* | **TX Activity LED** | *GPIO 15 (Pin 28)*| *Not Used on CCU* |

---

## 3. Power Distribution & Resistor Specs

### A. Power Lines
- **5V Vin Rail**: Feeds ESP32 #1 Vin, ESP32 #2 Vin, and MQ135 VCC.
- **3.3V Rail**: Feeds DHT11/22 VCC, MPU6050 VCC, NEO-6M GPS VCC, and LoRa SX1278 VCC.
- **Common GND**: All components MUST share a unified Ground line.

### B. Resistor Divider Circuit for Battery ADC (GPIO 35)
```text
  Li-Ion Battery (+) --- [ 10k Ohm (R1) ] ---*--- [ 10k Ohm (R2) ] --- GND
                                              |
                                      ESP32 #1 GPIO 35
```
$$\text{V}_{\text{ADC}} = \frac{\text{V}_{\text{Battery}}}{2}$$
For a full battery ($4.2\text{V}$), input voltage to GPIO 35 is $2.1\text{V}$ (safe for ESP32 ADC).

---

## 4. Hardware Assembly Verification

1. **Verify Parallel UART Connections**:
   - SPU GPIO 17 (TX2) $\rightarrow$ CCU GPIO 16 (RX2)
   - SPU GPIO 16 (RX2) $\leftarrow$ CCU GPIO 17 (TX2)
2. **Verify Common Ground**: Ensure ground wire connects SPU GND pin 14 to CCU GND pin 14.
3. **Connect Antenna**: Always attach the 433 MHz antenna to the SX1278 module before powering on the CCU.
