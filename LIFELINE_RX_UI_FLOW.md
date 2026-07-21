# LIFELINE RX — Display UI Flow & Design Reference
> **Hardware:** ESP32 · 16×2 I²C LCD (0x27) · 16 cols × 2 rows  
> **Firmware:** v3.1.0 PRO  
> **File:** `DisplayUI.cpp / DisplayUI.h`

---

## Table of Contents
1. [Overview & Screen States](#1-overview--screen-states)
2. [Boot Screen](#2-boot-screen)
3. [WiFi Splash Screen](#3-wifi-splash-screen)
4. [Home / Idle Screen](#4-home--idle-screen)
5. [Alert / Signal Received Screen](#5-alert--signal-received-screen)
6. [No WiFi Screen](#6-no-wifi-screen)
7. [WiFi Setup Countdown Screen](#7-wifi-setup-countdown-screen)
8. [WiFi Portal Active Screen](#8-wifi-portal-active-screen)
9. [Buzzer & LED Behaviour Table](#9-buzzer--led-behaviour-table)
10. [Complete State Flow Diagram](#10-complete-state-flow-diagram)
11. [LCD Pixel Layout Reference](#11-lcd-pixel-layout-reference)

---

## 1. Overview & Screen States

```
┌─────────────────────────────────────────────────────────┐
│                  SCREEN STATE MACHINE                   │
│                                                         │
│  SCREEN_BOOT  →  SCREEN_IDLE  ⇄  SCREEN_ALERT          │
│                       ↑                                 │
│            (WiFi splash shown between them)             │
│                                                         │
│  SCREEN_NO_WIFI  →  SCREEN_IDLE  (1 min timeout)        │
│  SCREEN_NO_WIFI  →  SCREEN_PORTAL_COUNTDOWN → PORTAL   │
└─────────────────────────────────────────────────────────┘
```

| State ID | Screen Name | Trigger | Duration |
|---|---|---|---|
| `SCREEN_BOOT` | Boot Screen | Power ON / Reset | ~2 seconds |
| *(transient)* | WiFi Connected Splash | Boot complete + WiFi OK | ~1.5 seconds |
| *(transient)* | No WiFi Screen | Boot complete + no WiFi | Until input or 60 s |
| `SCREEN_IDLE` | Home / Idle Screen | After boot/WiFi phase | Indefinite |
| `SCREEN_ALERT` | Alert Screen | LoRa packet received | 30 seconds then auto-return |
| *(transient)* | Portal Countdown | GPIO 14 held 3 s | 3-second progress bar |
| `SCREEN_PORTAL` | WiFi Setup Portal | Portal countdown ends | Until saved / 3 min timeout |

---

## 2. Boot Screen

**Trigger:** Power on or hardware reset  
**Duration:** ~2 seconds (`BOOT_DISPLAY_TIME = 1000 ms` + animation frames)  
**Buzzer:** Single short beep (1500 Hz, 80 ms) on first frame  
**LEDs:** Both OFF during boot init

### LCD Layout

```
┌────────────────┐
│  LIFELINE RX   │   ← Row 0: Centered, padded
│  Booting...    │   ← Row 1: Animates dots every 300 ms
└────────────────┘
```

### Dot Animation Sequence (300 ms per frame)

| Frame | Row 1 Display | Elapsed |
|---|---|---|
| 0 | `   Booting     ` | 0–299 ms |
| 1 | `   Booting.    ` | 300–599 ms |
| 2 | `   Booting..   ` | 600–899 ms |
| 3 | `   Booting...  ` | 900–1299 ms |

> After `BOOT_DISPLAY_TIME` elapses, boot animation ends and system moves to the WiFi check phase.

### Exact LCD character map (16 cols × 2 rows)

```
Col: 0123456789012345
     ┌────────────────┐
R0:  │  LIFELINE RX  │
R1:  │   Booting...  │
     └────────────────┘
```

---

## 3. WiFi Splash Screen

**Trigger:** Immediately after boot completes  
**Duration:** ~1.5 seconds (informational, then auto-advances to Idle)  
**Buzzer:** Double short beep (WiFi success confirmation)  
**LEDs:** WiFi LED (GPIO 21) turns ON solid green

### Case A — WiFi Connected

```
Col: 0123456789012345
     ┌────────────────┐
R0:  │ WiFi Connected!│
R1:  │ IP:192.168.1.5 │   ← Actual IP address
     └────────────────┘
```

### Case B — WiFi Connecting (tries up to 3 stored networks)

```
Col: 0123456789012345
     ┌────────────────┐
R0:  │ WiFi Connecting│
R1:  │ 1/2:HomeNet    │   ← Network index / total : SSID
     └────────────────┘
```

> After all attempts fail, transitions to **No WiFi Screen** instead of Idle.

---

## 4. Home / Idle Screen

**Trigger:** After boot + WiFi phase completes  
**Duration:** Indefinite — waits for LoRa signal  
**Buzzer:** Silent  
**LEDs:** WiFi LED = solid (ON if connected, OFF if not); Data LED = OFF  

### LCD Layout

```
Col: 0123456789012345
     ┌────────────────┐
R0:  │LIFELINE RX  O  │   ← O = radar glyph pulses every 600ms
R1:  │433MHz  Alt:0   │   ← Frequency + cumulative alert count
     └────────────────┘
```

### Row Breakdown

| Row | Content | Description |
|---|---|---|
| R0 col 0–10 | `LIFELINE RX ` | Static label |
| R0 col 13 | `[*]` or `o` | Radar glyph pulses every 600 ms |
| R1 col 0–5 | `433MHz` | LoRa frequency label (static) |
| R1 col 8–15 | `Alt:XXXX` | Total alerts received since power-on |

### Radar Pulse Animation

```
t=0ms    → [*]  (custom radar glyph — CGRAM slot 0)
t=600ms  →  o   (lowercase letter 'o')
t=1200ms → [*]
t=1800ms →  o   ... repeating indefinitely
```

> This visual pulse indicates the device is **actively listening** for incoming LoRa transmissions.

---

## 5. Alert / Signal Received Screen

**Trigger:** Valid LoRa packet parsed (`parseLoRaPacket()` returns true)  
**Duration:** 30 seconds (`ALERT_DISPLAY_TIME`), then auto-returns to Idle  
**Buzzer:** Alert tone based on priority (see §9)  
**LEDs:** Data LED (GPIO 13) blinks ON for 250 ms then OFF; WiFi LED unchanged

### LCD Layout

```
Col: 0123456789012345
     ┌────────────────┐
R0:  │[!] A EMERGENCY │   ← Warning icon + Alert Code + Short Alert Name
R1:  │TX#001 -65dBm CR│   ← TX ID + RSSI + Priority
     └────────────────┘
```

### Row 0 — Alert Identity

| Col | Field | Example | Description |
|---|---|---|---|
| 0 | `[!]` | Warn glyph | Custom warn glyph (CGRAM slot 4) |
| 1 | Space | ` ` | Separator |
| 2 | Alert Code | `A` | Single letter A–O (maps to alert index 0–14) |
| 3 | Space | ` ` | Separator |
| 4–15 | Alert Name (short) | `EMERGENCY    ` | Up to 12 chars, padded |

### Row 1 — Telemetry + Status

| Col | Field | Example | Description |
|---|---|---|---|
| 0–5 | `TX#XXX` | `TX#001` | Transmitter device ID (0-padded 3 digits) |
| 6 | Space | ` ` | Separator |
| 7–12 | `XXXdBm` | `-65dBm` | RSSI signal strength |
| 13 | Space | ` ` | Separator |
| 14–15 | Priority label | `CR` | CRIT/HIGH/MED/OK/INFO |

### Priority Labels

| Priority | Display | Alert Tone |
|---|---|---|
| 0 — Critical | `CRIT` | 3× beep at 2500 Hz, 100 ms each |
| 1 — High | `HIGH` | 2× beep at 2200 Hz, 150 ms each |
| 2 — Medium | `MED ` | 1× beep at 2000 Hz, 200 ms |
| 3 — OK | `OK  ` | 1× soft beep |
| 4 — Info | `INFO` | 1× soft beep |

### API Upload Status

| WiFi Status | API Result | Row 1 suffix |
|---|---|---|
| Connected | HTTP 200 OK | `SENT` at far right |
| Connected | HTTP error | `ERR ` at far right |
| Not connected | N/A | Blank / no suffix |

### All 15 Alert Codes

| Code | Alert Name (short) | Priority | Buzzer |
|---|---|---|---|
| A | EMERGENCY | CRIT | 3× 2500Hz |
| B | MEDICAL EMERG | CRIT | 3× 2500Hz |
| C | MED SHORTAGE | MED | 1× 2000Hz |
| D | EVACUATION | CRIT | 3× 2500Hz |
| E | STATUS OK | OK | 1× soft |
| F | INJURY | HIGH | 2× 2200Hz |
| G | FOOD SHORT | MED | 1× 2000Hz |
| H | WATER SHORT | MED | 1× 2000Hz |
| I | WEATHER | MED | 1× 2000Hz |
| J | LOST PERSON | HIGH | 2× 2200Hz |
| K | ANIMAL ATTK | HIGH | 2× 2200Hz |
| L | LANDSLIDE | HIGH | 2× 2200Hz |
| M | SNOW STORM | HIGH | 2× 2200Hz |
| N | EQUIP FAIL | MED | 1× 2000Hz |
| O | OTHER | INFO | 1× soft |

### Alert Screen Examples

**EMERGENCY (Critical):**
```
Col: 0123456789012345
     ┌────────────────┐
R0:  │[!]A EMERGENCY  │
R1:  │TX#001 -65dBm CR│
     └────────────────┘
```

**MEDICAL EMERGENCY (Critical, no WiFi):**
```
Col: 0123456789012345
     ┌────────────────┐
R0:  │[!]B MEDICAL EM │
R1:  │TX#003 -78dBm CR│
     └────────────────┘
```

**STATUS OK (OK priority):**
```
Col: 0123456789012345
     ┌────────────────┐
R0:  │[!]E STATUS OK  │
R1:  │TX#001 -60dBm OK│
     └────────────────┘
```

> After 30 s with no new packet → auto-returns to **Home/Idle Screen**.  
> New packet while showing → updates in place + resets 30 s timer.

---

## 6. No WiFi Screen

**Trigger:** Boot completes with no stored credentials OR all stored networks failed  
**Duration:** 60 seconds auto-advance to Idle, or user input  
**Buzzer:** Two descending beeps (failure tone)  
**LEDs:** WiFi LED OFF; Data LED OFF

### LCD Layout

```
Col: 0123456789012345
     ┌────────────────┐
R0:  │ No Internet!   │
R1:  │1x=Skip 3s=Setup│
     └────────────────┘
```

### User Actions

| Action | Description | Next Screen |
|---|---|---|
| **Short press** (< 1 s) | Single confirm beep | → Idle Screen (no WiFi mode) |
| **Hold 3 seconds** | Countdown bar starts | → WiFi Setup Countdown |
| **No input for 60 s** | Soft beep, auto-advance | → Idle Screen (no WiFi mode) |

---

## 7. WiFi Setup Countdown Screen

**Trigger:** User holds GPIO 14 button for 3 seconds (from No WiFi or Idle screen)  
**Duration:** 3-second countdown while button is held  
**Buzzer:** One short beep per second (tick-tick-tick at 2000 Hz)  
**LEDs:** WiFi LED flashes slowly (500 ms on / 500 ms off)

### LCD Layout — Progress Bar filling left-to-right

```
Frame at t=1s (33% filled):
Col: 0123456789012345
     ┌────────────────┐
R0:  │GPIO14 Btn Held │
R1:  │#####           │   ← 5 of 16 block chars filled
     └────────────────┘

Frame at t=2s (66% filled):
     ┌────────────────┐
R0:  │GPIO14 Btn Held │
R1:  │##########      │   ← 10 of 16 block chars filled
     └────────────────┘

Frame at t=3s (100% filled):
     ┌────────────────┐
R0:  │GPIO14 Btn Held │
R1:  │################│   ← 16 of 16 block chars filled
     └────────────────┘
```

> **Progress bar formula:**  
> `filledChars = (elapsed_ms * 16) / 3000`  
> Each block = ~187.5 ms; uses LCD `0xFF` (full block char)

> If button released early → countdown cancels, returns to previous screen.

---

## 8. WiFi Portal Active Screen

**Trigger:** 3-second countdown completes  
**Duration:** Up to 3 minutes (`WIFI_PORTAL_TIMEOUT = 180000 ms`) or until saved  
**Buzzer:** Confirmation beep on portal open (1800 Hz, 150 ms)  
**LEDs:** WiFi LED fast-blink (100 ms on / 100 ms off — portal active indicator)

### LCD Layout

```
Col: 0123456789012345
     ┌────────────────┐
R0:  │AP:LifeLine-RX  │   ← Access Point SSID
R1:  │192.168.4.1     │   ← Connect & navigate to this IP
     └────────────────┘
```

> Connect phone/PC to `LifeLine-RX-Setup` WiFi → open browser → go to `192.168.4.1` → add up to 3 WiFi networks → Save → Device reboots and reconnects.

---

## 9. Buzzer & LED Behaviour Table

### Buzzer Summary

| Event | Frequency | Duration | Pattern |
|---|---|---|---|
| Boot startup | 1500 Hz | 80 ms | Single pip |
| WiFi connect success | 1000→1500 Hz | 80+80 ms | Rising double pip |
| WiFi failed | 1500→1000 Hz | 80+80 ms | Falling double pip |
| Portal countdown tick | 2000 Hz | 50 ms | 1 beep per second for 3 s |
| Portal open | 1800 Hz | 150 ms | Single medium beep |
| No-WiFi skip (short press) | 1500 Hz | 50 ms | Single soft pip |
| Alert — CRITICAL (0) | 2500 Hz | 100 ms | 3× rapid beeps (120 ms gap) |
| Alert — HIGH (1) | 2200 Hz | 150 ms | 2× beeps (180 ms gap) |
| Alert — MED / OK / INFO | 2000 Hz | 200 ms | 1× single beep |
| Return to Idle auto-timeout | 1200 Hz | 80 ms | Soft single pip |

### LED Summary

| LED | GPIO | Condition | State |
|---|---|---|---|
| WiFi LED (Green) | 21 | WiFi connected | Solid ON |
| WiFi LED (Green) | 21 | No WiFi | OFF |
| WiFi LED (Green) | 21 | Connecting / countdown | Slow blink (500 ms) |
| WiFi LED (Green) | 21 | Portal active | Fast blink (100 ms) |
| Data LED (Red) | 13 | LoRa packet received | 250 ms ON pulse |
| Data LED (Red) | 13 | Idle / no activity | OFF |

---

## 10. Complete State Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                     LIFELINE RX UI STATE MACHINE                    │
└─────────────────────────────────────────────────────────────────────┘

              [ POWER ON / RESET ]
                       │
                       ▼
             ┌─────────────────┐
             │   BOOT SCREEN   │  Buzzer: 1× pip (1500Hz, 80ms)
             │  LIFELINE RX    │  LEDs: both OFF
             │  Booting...     │  Duration: ~2 seconds
             └────────┬────────┘
                      │ Boot complete
                      ▼
            ┌──────────────────────┐
            │  WiFi Check Phase    │  Load stored credentials
            └─────────┬────────────┘
                      │
          ┌───────────┴────────────┐
    Credentials?                    │
       YES                         NO
          │                         │
          ▼                         ▼
  ┌───────────────┐        ┌────────────────────┐
  │  Try connect  │        │   NO WIFI SCREEN   │  Buzzer: 2× descending
  │  (up to 3     │        │  "No Internet!"    │  LEDs: both OFF
  │   networks,   │        │  "1x=Skip 3s=Setup"│
  │   8s timeout) │        └──────────┬─────────┘
  └───────┬───────┘                   │
          │                ┌──────────┼───────────┐
   Connected?            Short      60s          Hold
    YES   NO            press      timeout       3 sec
     │     │              │           │             │
     │     │              ▼           ▼             ▼
     │   ┌────────┐    [IDLE]      [IDLE]     ┌──────────────┐
     │   │WiFi    │   (no net)    (no net)    │  COUNTDOWN   │
     │   │Failed  │                           │  SCREEN      │
     │   │Screen  │                           │ GPIO14 Held  │
     │   └────┬───┘                           │ [##########] │
     │        │                               │ bar fills 3s │
     ▼        ▼                               └──────┬───────┘
  ┌────────────────────┐                            │ 3s done?
  │ WIFI CONNECTED     │                     YES ───┘
  │ SPLASH SCREEN      │                     │
  │ "WiFi Connected!"  │                     ▼
  │ "IP:xxx.xxx.x.xx"  │         ┌───────────────────────┐
  └────────┬───────────┘         │  WIFI PORTAL SCREEN   │
           │ 1.5s                │  AP:LifeLine-RX-Setup  │
           │                     │  192.168.4.1           │
           ▼                     └───────────┬───────────┘
  ┌─────────────────────────────────────┐    │
  │          HOME / IDLE SCREEN         │◄───┤ (save & reboot
  │  "LIFELINE RX   [*]"                │    │  or 3min timeout)
  │  "433MHz  Alt:NNN"                  │    │
  │   radar glyph pulses every 600ms    │    │
  └──────────────┬──────────────────────┘    │
                 │                            │
         LoRa packet arrives                 │
                 │                            │
                 ▼                            │
  ┌─────────────────────────────────────┐    │
  │          ALERT SCREEN               │    │
  │  "[!] X ALERT NAME      "           │    │
  │  "TX#NNN -NNdBm   PRIORITY"         │    │
  │                                     │    │
  │  Buzzer: priority tone              │    │
  │  Data LED: 250ms blink              │    │
  │  API push if WiFi connected         │    │
  └──────────────┬──────────────────────┘    │
                 │                            │
     ┌───────────┴────────────┐              │
     │                        │              │
   New packet             30s timeout        │
   received                   │              │
     │                        ▼              │
     │              ┌──────────────────┐     │
     │              │  HOME / IDLE     │─────┘
     │              │  SCREEN          │
     ▼              └──────────────────┘
  Update alert
  on screen
  (reset 30s timer)
```

---

## 11. LCD Pixel Layout Reference

All screens are 16 columns × 2 rows. Column index: 0–15. Row index: 0–1.

### Custom CGRAM Glyphs (stored in LCD RAM slots 0–4)

| Slot | Symbol | Usage | Pattern |
|---|---|---|---|
| 0 | Radar `[*]` | Idle screen pulse, R0 col 13 | `01110 10001 00100 01010` |
| 1 | Bell | Reserved for history / alerts | `00100 01110 11111 00100` |
| 2 | Checkmark | Reserved for confirm states | `00001 00011 10110 11100` |
| 3 | Signal bars | Reserved for RSSI visual | `00001 00101 10101 00000` |
| 4 | Warning `[!]` | Alert screen R0 col 0 | `00100 01110 11111 11011` |

### Screen Quick Reference Matrix

| Screen | Row 0 (16 chars) | Row 1 (16 chars) |
|---|---|---|
| **Boot** | `  LIFELINE RX  ` | `   Booting...  ` |
| **WiFi OK splash** | `WiFi Connected!` | `IP:192.168.x.x ` |
| **WiFi connecting** | `WiFi Connecting` | `1/2:NetworkName` |
| **WiFi failed** | `WiFi Failed!   ` | `Open Setup AP..` |
| **Idle (Home)** | `LIFELINE RX  [*]` | `433MHz  Alt:NNN` |
| **Alert** | `[!] X ALERT NAME  ` | `TX#NNN -NNdBm PRI` |
| **No WiFi** | `No Internet!   ` | `1x=Skip 3s=Setup` |
| **Countdown** | `GPIO14 Btn Held` | `################` |
| **Portal active** | `AP:LifeLine-RX ` | `192.168.4.1    ` |

---

*Document generated for LIFELINE RX v3.1.0 PRO · ESP32 + 16×2 I²C LCD*  
*Last updated: 2026-07-21*
