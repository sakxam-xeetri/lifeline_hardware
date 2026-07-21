#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

/**
 * @brief Checks VPS endpoint for a newer firmware version manifest (version.json).
 *        If a newer version exists, streams the .bin payload directly over HTTPS to ESP32 OTA flash partition,
 *        updates 16x2 LCD with real-time progress, and reboots device upon success.
 * @return bool True if update was executed, False if no update or check failed/skipped.
 */
bool checkAndPerformOTA();

#endif // OTA_MANAGER_H
