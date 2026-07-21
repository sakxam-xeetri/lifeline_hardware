#include "OTAManager.h"
#include "Config.h"
#include "DisplayUI.h"
#include "BuzzerLED.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>

// Static Helper: Compare semantic versions (e.g. "3.1.0 PRO" vs "3.2.0")
static bool isVersionNewer(String currentVer, String newVer) {
    currentVer.trim();
    newVer.trim();
    
    if (currentVer.startsWith("v") || currentVer.startsWith("V")) currentVer = currentVer.substring(1);
    if (newVer.startsWith("v") || newVer.startsWith("V")) newVer = newVer.substring(1);
    
    int spaceCur = currentVer.indexOf(' ');
    if (spaceCur != -1) currentVer = currentVer.substring(0, spaceCur);
    
    int spaceNew = newVer.indexOf(' ');
    if (spaceNew != -1) newVer = newVer.substring(0, spaceNew);

    int curMajor = 0, curMinor = 0, curPatch = 0;
    int newMajor = 0, newMinor = 0, newPatch = 0;

    sscanf(currentVer.c_str(), "%d.%d.%d", &curMajor, &curMinor, &curPatch);
    sscanf(newVer.c_str(), "%d.%d.%d", &newMajor, &newMinor, &newPatch);

    if (newMajor > curMajor) return true;
    if (newMajor < curMajor) return false;

    if (newMinor > curMinor) return true;
    if (newMinor < curMinor) return false;

    return newPatch > curPatch;
}

// Static Helper: Extract JSON value string by key
static String extractJSONValue(const String& json, const String& key) {
    String searchKey = "\"" + key + "\":";
    int keyIdx = json.indexOf(searchKey);
    if (keyIdx == -1) {
        searchKey = "\"" + key + "\" :";
        keyIdx = json.indexOf(searchKey);
    }
    if (keyIdx == -1) return "";

    int valStart = json.indexOf('"', keyIdx + searchKey.length());
    if (valStart == -1) return "";
    valStart += 1;

    int valEnd = json.indexOf('"', valStart);
    if (valEnd == -1) return "";

    return json.substring(valStart, valEnd);
}

bool checkAndPerformOTA() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[OTA] Skipping OTA check: Wi-Fi not connected."));
        return false;
    }

    Serial.println(F("\n═══════════════════════════════════════════════════════════"));
    Serial.println(F("[OTA] Checking VPS server for remote firmware updates..."));
    Serial.printf("[OTA] Target Manifest URL: %s\n", OTA_VERSION_URL);
    Serial.printf("[OTA] Current Device Firmware: %s\n", FIRMWARE_VERSION);
    Serial.println(F("═══════════════════════════════════════════════════════════"));

    // Update 16x2 LCD UI
    currentScreen = SCREEN_OTA;
    drawOTACheckingScreen();

    WiFiClientSecure client;
    client.setInsecure(); // Secure SSL connection without hardcoded CA certificate

    HTTPClient http;
    http.setTimeout(OTA_CHECK_TIMEOUT_MS);
    
    if (!http.begin(client, OTA_VERSION_URL)) {
        Serial.println(F("[OTA] ERROR: Failed to initialize HTTPS connection to VPS."));
        return false;
    }

    // Telemetry headers sent to VPS
    http.addHeader("User-Agent", "LifeLine-RX-BaseStation/3.1");
    http.addHeader("X-ESP32-MAC", WiFi.macAddress());
    http.addHeader("X-ESP32-Firmware", FIRMWARE_VERSION);
    http.addHeader("X-ESP32-FreeHeap", String(ESP.getFreeHeap()));

    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OTA] HTTP check failed. Code: %d (%s)\n", httpCode, http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    Serial.println(F("[OTA] Manifest received from VPS:"));
    Serial.println(payload);

    String latestVersion = extractJSONValue(payload, "version");
    String binaryUrl = extractJSONValue(payload, "url");

    if (latestVersion.length() == 0 || binaryUrl.length() == 0) {
        Serial.println(F("[OTA] ERROR: Malformed JSON manifest. Missing 'version' or 'url'."));
        return false;
    }

    Serial.printf("[OTA] Server Version: '%s' | Binary URL: '%s'\n", latestVersion.c_str(), binaryUrl.c_str());

    if (!isVersionNewer(FIRMWARE_VERSION, latestVersion)) {
        Serial.println(F("[OTA] Device is running the latest firmware. No update needed."));
        return false;
    }

    // Update Found! Prepare LCD and initiate download
    Serial.println(F("[OTA] *** NEW FIRMWARE AVAILABLE! INITIATING UPDATE ***"));
    
    playBootTone(); // Audio beep notification
    drawOTAFoundScreen(latestVersion);
    delay(1500); // Allow user to read new version on LCD

    // Setup HTTPUpdate progress callbacks for 16x2 LCD
    httpUpdate.onStart([]() {
        Serial.println(F("[OTA] Download started..."));
        drawOTAProgressScreen(0);
    });

    httpUpdate.onProgress([](int current, int total) {
        if (total > 0) {
            int percent = (current * 100) / total;
            static int lastPercent = -1;
            if (percent != lastPercent) {
                lastPercent = percent;
                Serial.printf("[OTA] Progress: %d / %d bytes (%d%%)\n", current, total, percent);
                drawOTAProgressScreen(percent);
            }
        }
    });

    httpUpdate.onEnd([]() {
        Serial.println(F("[OTA] Download complete! Writing flash..."));
        drawOTASuccessScreen();
    });

    httpUpdate.onError([](int err) {
        Serial.printf("[OTA] ERROR: Update failed (Code %d)\n", err);
    });

    // Execute firmware streaming & partition update
    t_httpUpdate_return ret = httpUpdate.update(client, binaryUrl);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("[OTA] Update Failed! Error (%d): %s\n", 
                          httpUpdate.getLastError(), 
                          httpUpdate.getLastErrorString().c_str());
            drawOTAFailedScreen("Flash Failed!");
            delay(2000);
            return false;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println(F("[OTA] No updates performed."));
            return false;

        case HTTP_UPDATE_OK:
            Serial.println(F("[OTA] SUCCESS: Firmware update complete! Rebooting device now..."));
            delay(1000);
            ESP.restart();
            return true;
    }

    return false;
}
