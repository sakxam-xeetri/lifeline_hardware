#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include "Config.h"
#include "BuzzerLED.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

extern Adafruit_ST7789 tft;

void initDisplay();

uint16_t getPriorityColor(uint8_t priority);
uint16_t getAlertColor(int index);
void updateMenuScroll();

void drawCenteredText(const char* text, int y, uint8_t textSize, uint16_t color);
void drawRightText(const char* text, int y, uint8_t textSize, uint16_t color);
void drawGradientH(int x, int y, int w, int h, uint16_t colorStart, uint16_t colorEnd);
void drawGradientV(int x, int y, int w, int h, uint16_t colorTop, uint16_t colorBottom);
void drawPremiumCard(int x, int y, int w, int h, uint16_t bgColor, uint16_t borderColor, bool hasGlow = false);
void drawGlowCircle(int cx, int cy, int r, uint16_t color);
void drawStatusIndicator(int x, int y, int r, uint16_t color, bool active = true);

void drawHeader(const char* title);
void drawFooter(const char* hints);
void drawKeyBadge(int x, int y, char key, const char* label, uint16_t keyColor);
void drawSeparator(int y);

void drawBootScreen();
void updateMenuSelection(int oldIndex, int newIndex);
void drawMenuScreen();
void drawConfirmScreen();
void drawSendingScreen();
void drawResultScreen();
void drawSystemInfoScreen();
void drawUserManualScreen();

#endif // DISPLAY_UI_H
