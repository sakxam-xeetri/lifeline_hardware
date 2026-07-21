#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include "Config.h"
#include <LiquidCrystal_I2C.h>

// Global Display Instance
extern LiquidCrystal_I2C lcd;

// Global State Variables (declared extern, defined in DisplayUI.cpp or .ino)
extern ScreenState currentScreen;
extern ScreenState previousScreenBeforePress;
extern unsigned long bootStartTime;
extern unsigned long noWiFiStartTime;
extern unsigned long alertReceivedTime;
extern unsigned long lastPulseTime;
extern uint8_t pulseState;
extern uint8_t bootDotState;

extern int lastDeviceId;
extern int lastAlertIndex;
extern int lastRssi;

extern AlertRecord alertHistory[HISTORY_MAX_ITEMS];
extern int historyCount;
extern int historyScrollOffset;
extern int totalAlertsReceived;

// Functions
void initDisplay();
void printLCDLine(uint8_t row, const String& text);
void printLCDCenter(uint8_t row, const String& text);

// Boot Screen
void drawBootScreen();
bool updateBootAnimation();

// Idle Screen
void drawIdleScreen();
void updateIdleAnimation();

// Alert Screen
void drawAlertScreen(int deviceId, int alertIndex, int rssi);
bool shouldReturnToIdle();

// WiFi Screens
void drawNoWiFiScreen();
void drawWiFiConnectingScreen(const String& ssid, int currentIdx, int totalCount);
void drawWiFiConnectedScreen(const String& ip);
void drawWiFiFailedScreen();
void drawCountdownScreen(int remainingSec);
void drawProgressCountdownScreen(unsigned long elapsedMs, unsigned long totalMs);
void drawWiFiPortalScreen();

// History
void addToHistory(int deviceId, int alertIndex, int rssi);

// Helper functions
char getAlertCode(int index);
void initNTPTime();
bool getFormattedTimeStr(char* buffer, size_t maxLen);

#endif // DISPLAY_UI_H
