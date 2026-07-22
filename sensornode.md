```text
You are a Senior Embedded Systems Architect, IoT Engineer, and ESP-IDF Expert.

Your task is to completely redesign the hardware architecture of the existing LifeLine Sensor Node while keeping the existing Base Station and Web Dashboard architecture compatible.

DO NOT rewrite the entire project from scratch.

Instead, redesign the transmitter node into a modular, scalable architecture suitable for real-world deployment.

========================================
PROJECT OVERVIEW
========================================

LifeLine is an AI-powered Off-Grid Disaster Monitoring and Emergency Response System designed for remote villages, forests, disaster-prone areas, trekking routes, and places where cellular communication is unavailable.

The communication architecture remains:

Sensor Node
        ↓
LoRa
        ↓
Base Station
        ↓
Internet
        ↓
Cloud Dashboard
        ↓
Email / SMS / Push Notification

Only the Sensor Node architecture is changing.

========================================
NEW SENSOR NODE ARCHITECTURE
========================================

Instead of using a single ESP32 for everything, redesign the node using TWO ESP32 boards.

----------------------------------------
ESP32 #1
Sensor Processing Unit (SPU)
----------------------------------------

This ESP32 is responsible ONLY for sensor acquisition, local processing, filtering, and emergency detection.

It DOES NOT communicate using LoRa.

Its responsibilities include:

• Reading all connected sensors
• Filtering noisy sensor values
• Sensor calibration
• Environmental analysis
• Detecting emergency conditions
• Creating structured sensor data
• Sending processed data to the Communication Controller using UART.

========================================

Connected Sensors

GPS Module

Read:

• Latitude
• Longitude
• Altitude
• Speed
• UTC Time
• Number of Satellites
• GPS Fix Status

----------------------------------------

Environmental Sensor



DHT11 

Read:

• Temperature
• Humidity
• Atmospheric Pressure

Detect

• Heat Wave
• Cold Wave
• High Humidity
• Dry Environment

----------------------------------------

MPU6050

Read

Accelerometer

Gyroscope

Internal Temperature

Detect

• Motion

• Fall

• Free Fall

• Sudden Impact

• Long Inactivity

• Device Tilt

• Abnormal Vibration

• Earthquake

Use moving average and threshold filtering.

Do not trigger false alarms.

----------------------------------------

MQ135

Read

Gas Concentration

Detect

• Smoke

• Air Pollution

• Poor Air Quality

• Gas Leakage

• Possible Fire

----------------------------------------

B

========================================

Automatic Emergency Detection

The Sensor ESP32 should automatically classify emergencies.

Examples

Fire

High Gas

+

High Temperature

↓

Fire Alert

Earthquake

Continuous vibration

↓

Earthquake Alert

Extreme Heat

↓

Heat Alert

Extreme Cold

↓

Cold Alert

High Humidity

↓

Weather Warning

Low Battery

↓

Maintenance Alert

Manual SOS Button

↓

Emergency SOS

========================================

Sensor Fusion

Do NOT simply transmit raw sensor values.

Fuse sensor data to generate intelligent events.

Example

Temperature

Humidity

Gas

↓

Possible Forest Fire

or

MPU6050

+

Pressure Change

↓

Possible Landslide

========================================

Health Score

Generate

Node Health Score

Environmental Risk Score

Battery Health

Emergency Priority

These values are sent to the Master ESP32.

========================================

UART Communication

The Sensor ESP32 communicates ONLY with the Master ESP32 through UART.

Create a structured packet.

Example

Node ID


Temperature

Humidity

Pressure

GPS

Acceleration

Gyroscope

Health Score

Risk Score

Emergency Type

Emergency Priority

Checksum

Packet Version

========================================
ESP32 #2
Communication Controller
========================================

This ESP32 NEVER reads sensors directly.

Its responsibilities

Receive UART packets.

Validate packet integrity.

Generate LoRa packets.

AES Encryption.

LoRa Communication.

Mesh Routing.

Acknowledgements.

Retry Failed Packets.

Sleep Management.

OTA Updates.

Communication Diagnostics.

Node Authentication.

Packet Logging.

Forward packets to the Base Station.

========================================

Communication Logic

Sensor ESP32

↓

UART

↓

Communication ESP32

↓

LoRa

↓

Relay Nodes

↓

Base Station

↓

Internet

↓

Dashboard

========================================

Code Architecture

Separate firmware into independent modules.

sensor_manager

gps_manager

environment_manager

mpu_manager

gas_manager


emergency_detector

packet_builder

uart_manager

communication_manager

lora_manager

config

utils

main

Avoid writing everything inside main.cpp.

========================================

Configuration

Create config.h

All thresholds must be configurable.

Heat Threshold

Cold Threshold

Humidity Threshold

Gas Threshold

Fire Threshold

Earthquake Threshold

UART Baudrate

LoRa Interval

GPS Interval

Sensor Sampling Rate

========================================

Power Optimization

Deep Sleep

Watchdog Timer

Non-blocking code

FreeRTOS Tasks

Interrupt-based events

Minimal CPU usage

========================================


========================================

Engineering Goals

The final architecture should resemble an industrial IoT edge node rather than a hobby Arduino project.

The design must be

• Modular

• Scalable

• Fault Tolerant

• Maintainable

• Production Ready

• Easy to Debug

• Easy to Upgrade

• Compatible with ESP-IDF

• Compatible with the existing LifeLine Base Station and Dashboard

Do not modify the existing Base Station or Cloud architecture.

Only redesign the Sensor Node into a professional dual-ESP32 architecture with clean firmware separation and robust UART-to-LoRa communication.
```
