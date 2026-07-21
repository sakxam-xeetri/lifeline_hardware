#include "WiFiPortal.h"
#include "DisplayUI.h"
#include "BuzzerLED.h"
#include <WiFi.h>
#include <Preferences.h>
#include <Update.h>

WebServer wifiServer(80);
Preferences preferences;

// Define variables
bool wifiConnected = false;
bool portalActive = false;
unsigned long portalStartTime = 0;
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
int lastBeepSecond = -1;

WiFiNetwork storedNetworks[MAX_WIFI_NETWORKS];
int networkCount = 0;
String activeSSID = "";

// Forward declarations of server handlers
static void handlePortalRoot();
static void handlePortalSave();
static void handleUpdateRoot();
static void handleUpdateDone();
static void handleUpdateUpload();

void loadWiFiCredentials() {
    preferences.begin("lifeline", true);
    networkCount = 0;
    
    for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
        String keySSID = "ssid" + String(i + 1);
        String keyPass = "pass" + String(i + 1);
        String s = preferences.getString(keySSID.c_str(), "");
        String p = preferences.getString(keyPass.c_str(), "");
        
        if (s.length() > 0) {
            storedNetworks[networkCount].ssid = s;
            storedNetworks[networkCount].password = p;
            networkCount++;
        }
    }
    preferences.end();
    
    if (networkCount > 0) {
        activeSSID = storedNetworks[0].ssid;
        Serial.printf("[WIFI] Loaded %d WiFi network(s). Primary: %s\n", networkCount, activeSSID.c_str());
    } else {
        Serial.println(F("[WIFI] No stored WiFi networks found"));
    }
}

void saveWiFiCredentialsList(const WiFiNetwork nets[], int count) {
    preferences.begin("lifeline", false);
    for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
        String keySSID = "ssid" + String(i + 1);
        String keyPass = "pass" + String(i + 1);
        if (i < count && nets[i].ssid.length() > 0) {
            preferences.putString(keySSID.c_str(), nets[i].ssid);
            preferences.putString(keyPass.c_str(), nets[i].password);
        } else {
            preferences.remove(keySSID.c_str());
            preferences.remove(keyPass.c_str());
        }
    }
    preferences.end();
    
    loadWiFiCredentials();
}

bool connectToWiFiSilent() {
    if (networkCount == 0) {
        Serial.println(F("[WIFI] No networks configured for silent connect"));
        return false;
    }
    
    WiFi.mode(WIFI_STA);
    
    for (int i = 0; i < networkCount; i++) {
        Serial.printf("[WIFI] Silent connect attempt %d/%d to %s...\n", 
                      i + 1, networkCount, storedNetworks[i].ssid.c_str());
        WiFi.begin(storedNetworks[i].ssid.c_str(), storedNetworks[i].password.c_str());
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            activeSSID = storedNetworks[i].ssid;
            Serial.printf("[WIFI] Connected to %s! IP: %s\n", activeSSID.c_str(), WiFi.localIP().toString().c_str());
            return true;
        }
    }
    
    wifiConnected = false;
    Serial.println(F("[WIFI] All network connection attempts failed"));
    return false;
}

bool connectToWiFi() {
    if (networkCount == 0) {
        Serial.println(F("[WIFI] No credentials stored, launching open setup AP..."));
        drawWiFiFailedScreen();
        delay(1200);
        startWiFiPortal();
        return false;
    }
    
    WiFi.mode(WIFI_STA);
    
    for (int i = 0; i < networkCount; i++) {
        Serial.printf("[WIFI] Connecting to network %d/%d: %s...\n", 
                      i + 1, networkCount, storedNetworks[i].ssid.c_str());
        
        drawWiFiConnectingScreen(storedNetworks[i].ssid, i + 1, networkCount);
        WiFi.begin(storedNetworks[i].ssid.c_str(), storedNetworks[i].password.c_str());
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            activeSSID = storedNetworks[i].ssid;
            String ip = WiFi.localIP().toString();
            Serial.printf("[WIFI] Connected to %s! IP: %s\n", activeSSID.c_str(), ip.c_str());
            
            drawWiFiConnectedScreen(ip);
            delay(2000);
            return true;
        }
    }
    
    wifiConnected = false;
    Serial.println(F("[WIFI] All network connections failed - starting open setup portal"));
    
    drawWiFiFailedScreen();
    delay(1500);
    
    startWiFiPortal();
    return false;
}

void startWiFiPortal() {
    Serial.println(F("[WIFI] Starting OPEN WiFi configuration portal..."));
    
    WiFi.disconnect(true);
    delay(100);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID);
    
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[WIFI] Open AP started! SSID: '%s' (Free / No Password)\n", WIFI_AP_SSID);
    Serial.printf("[WIFI] Portal IP: %s\n", apIP.toString().c_str());
    
    wifiServer.on("/", handlePortalRoot);
    wifiServer.on("/save", HTTP_POST, handlePortalSave);
    wifiServer.on("/update", HTTP_GET, handleUpdateRoot);
    wifiServer.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
    wifiServer.begin();
    
    portalActive = true;
    portalStartTime = millis();
    
    drawWiFiPortalScreen();
}

void stopWiFiPortal() {
    if (!portalActive) return;
    
    Serial.println(F("[WIFI] Stopping portal..."));
    wifiServer.stop();
    WiFi.softAPdisconnect(true);
    portalActive = false;
    
    if (networkCount > 0) {
        connectToWiFi();
    }
    
    if (currentScreen != SCREEN_ALERT) {
        currentScreen = SCREEN_IDLE;
        drawIdleScreen();
    }
}

void checkWiFiPortalButton() {
    bool currentButtonState = digitalRead(WIFI_PORTAL_PIN);
    
    if (currentButtonState == LOW) { // Button on GPIO 14 active LOW
        if (!buttonPressed) {
            buttonPressed = true;
            buttonPressStartTime = millis();
            lastBeepSecond = -1;
        } else {
            unsigned long pressDuration = millis() - buttonPressStartTime;
            int secondsHeld = pressDuration / 1000;
            int remainingSec = 3 - secondsHeld;
            if (remainingSec < 1) remainingSec = 1;
            
            if (secondsHeld != lastBeepSecond && remainingSec >= 1 && remainingSec <= 3) {
                lastBeepSecond = secondsHeld;
                tone(BUZZER_PIN, 1800, 60);
            }
            
            if (!portalActive && pressDuration > 100) {
                drawCountdownScreen(remainingSec);
            }
            
            if (pressDuration >= LONG_PRESS_DURATION) {
                buttonPressed = false;
                tone(BUZZER_PIN, 2400, 200);
                
                if (portalActive) {
                    stopWiFiPortal();
                } else {
                    if (networkCount > 0) {
                        connectToWiFi();
                    } else {
                        startWiFiPortal();
                    }
                }
                
                if (!portalActive) {
                    if (currentScreen == SCREEN_IDLE) {
                        drawIdleScreen();
                    } else if (currentScreen == SCREEN_ALERT) {
                        drawAlertScreen(lastDeviceId, lastAlertIndex, lastRssi);
                    }
                }
            }
        }
    } else {
        if (buttonPressed) {
            buttonPressed = false;
            lastBeepSecond = -1;
            
            if (!portalActive && (millis() - buttonPressStartTime) > 200) {
                if (currentScreen == SCREEN_IDLE) {
                    drawIdleScreen();
                } else if (currentScreen == SCREEN_ALERT) {
                    drawAlertScreen(lastDeviceId, lastAlertIndex, lastRssi);
                }
            }
        }
    }
}

void handleWiFiPortal() {
    if (!portalActive) return;
    
    wifiServer.handleClient();
    
    if (millis() - portalStartTime >= WIFI_PORTAL_TIMEOUT) {
        Serial.println(F("[WIFI] Portal timeout, closing..."));
        stopWiFiPortal();
    }
}

// HTML and Endpoint Handlers
static String getPortalHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LifeLine RX WiFi Setup</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;background:#1a1a2e;color:#fff;margin:0;padding:20px;}";
    html += ".container{max-width:420px;margin:0 auto;background:#252542;padding:25px;border-radius:10px;}";
    html += "h1{color:#00d4ff;text-align:center;margin-bottom:5px;}";
    html += "<p class='subtitle'>Open WiFi Configuration Portal</p>";
    html += "h2{color:#ffb800;font-size:15px;margin-top:15px;margin-bottom:8px;border-bottom:1px solid #444;padding-bottom:5px;}";
    html += "input[type=text],input[type=password]{width:100%;padding:10px;margin:4px 0 14px;border:1px solid #444;border-radius:5px;background:#1a1a2e;color:#fff;box-sizing:border-box;}";
    html += "input[type=submit]{width:100%;padding:14px;background:#00d4ff;color:#000;border:none;border-radius:5px;cursor:pointer;font-weight:bold;font-size:16px;margin-top:15px;}";
    html += "input[type=submit]:hover{background:#00b8e6;}";
    html += ".ota-btn{display:block;text-align:center;margin-top:18px;padding:12px;background:#2d2d44;color:#00ff87;border-radius:5px;text-decoration:none;font-weight:bold;}";
    html += ".status{text-align:center;margin-top:20px;padding:10px;border-radius:5px;font-size:13px;background:#2d2d44;color:#a3b1c6;}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>LifeLine RX</h1>";
    html += "<p class='subtitle'>Open WiFi Configuration Portal</p>";
    html += "<form action='/save' method='POST'>";
    
    for (int i = 0; i < 3; i++) {
        String numStr = String(i + 1);
        String currentS = (i < networkCount) ? storedNetworks[i].ssid : "";
        String currentP = (i < networkCount) ? storedNetworks[i].password : "";
        
        html += "<h2>WiFi Network #" + numStr + (i == 0 ? " (Primary)" : "") + "</h2>";
        html += "<label>SSID Name:</label>";
        html += "<input type='text' name='ssid" + numStr + "' placeholder='Network name' value='" + currentS + "'>";
        html += "<label>Password:</label>";
        html += "<input type='password' name='pass" + numStr + "' placeholder='Password' value='" + currentP + "'>";
    }
    
    html += "<input type='submit' value='Save All Networks'>";
    html += "</form>";
    
    html += "<a class='ota-btn' href='/update'>⚡ OTA Firmware Update Page</a>";
    
    if (networkCount > 0) {
        html += "<div class='status'>Configured Networks: " + String(networkCount) + " / 3</div>";
    }
    
    html += "</div></body></html>";
    return html;
}

static String getUpdateHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LifeLine RX - OTA Update</title>";
    html += "<style>";
    html += "body{font-family:Arial;background:#1a1a2e;color:#fff;padding:25px;}";
    html += ".container{max-width:420px;margin:0 auto;background:#252542;padding:25px;border-radius:10px;}";
    html += "h1{color:#00d4ff;text-align:center;margin-bottom:10px;}";
    html += "<p>Upload compiled <b>firmware.bin</b> file to update LifeLine RX wirelessly over WiFi</p>";
    html += "input[type=file]{width:100%;padding:12px;margin:15px 0;background:#1a1a2e;color:#fff;border:1px solid #444;border-radius:5px;box-sizing:border-box;}";
    html += "input[type=submit]{width:100%;padding:14px;background:#00ff87;color:#000;border:none;border-radius:5px;font-weight:bold;font-size:16px;cursor:pointer;}";
    html += ".back{display:block;text-align:center;margin-top:20px;color:#00d4ff;text-decoration:none;font-weight:bold;}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1>OTA Firmware Update</h1>";
    html += "<p>Upload compiled <b>firmware.bin</b> file to update LifeLine RX wirelessly over WiFi</p>";
    html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
    html += "<input type='file' name='update' accept='.bin' required>";
    html += "<input type='submit' value='Upload & Flash Firmware'>";
    html += "</form>";
    html += "<a class='back' href='/'>← Back to WiFi Setup Portal</a>";
    html += "</div></body></html>";
    return html;
}

static void handlePortalRoot() {
    wifiServer.send(200, "text/html", getPortalHTML());
}

static void handlePortalSave() {
    WiFiNetwork newNets[3];
    int count = 0;
    
    for (int i = 0; i < 3; i++) {
        String s = wifiServer.arg("ssid" + String(i + 1));
        String p = wifiServer.arg("pass" + String(i + 1));
        s.trim();
        p.trim();
        if (s.length() > 0) {
            newNets[count].ssid = s;
            newNets[count].password = p;
            count++;
        }
    }
    
    saveWiFiCredentialsList(newNets, count);
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LifeLine RX - Saved</title>";
    html += "<style>body{font-family:Arial;background:#1a1a2e;color:#fff;text-align:center;padding:50px;}";
    html += ".success{background:#00ff87;color:#000;padding:20px;border-radius:10px;max-width:400px;margin:0 auto;}</style></head><body>";
    html += "<div class='success'><h2>WiFi Networks Saved!</h2>";
    html += "<p>Configured: " + String(count) + " network(s)</p>";
    html += "<p>Device will restart and connect...</p></div>";
    html += "</body></html>";
    
    wifiServer.send(200, "text/html", html);
    
    delay(2000);
    ESP.restart();
}

static void handleUpdateRoot() {
    wifiServer.send(200, "text/html", getUpdateHTML());
}

static void handleUpdateDone() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LifeLine RX - Flashed</title>";
    html += "<style>body{font-family:Arial;background:#1a1a2e;color:#fff;text-align:center;padding:50px;}";
    html += ".success{background:#00ff87;color:#000;padding:20px;border-radius:10px;max-width:400px;margin:0 auto;}</style></head><body>";
    html += "<div class='success'><h2>OTA Update Successful!</h2>";
    html += "<p>Firmware flashed successfully. Device is restarting...</p></div>";
    html += "</body></html>";
    
    wifiServer.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
}

static void handleUpdateUpload() {
    HTTPUpload& upload = wifiServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("[OTA] Update Start: %s\n", upload.filename.c_str());
        printLCDLine(0, "OTA Flashing...");
        printLCDLine(1, "Progress: 0%");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
        int percent = 0;
        if (Update.size() > 0) {
            percent = (Update.progress() * 100) / Update.size();
        }
        char prg[17];
        snprintf(prg, sizeof(prg), "Progress: %d%%", percent);
        printLCDLine(1, prg);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("[OTA] Success! Total size: %u B\n", upload.totalSize);
            printLCDLine(0, "OTA Success!");
            printLCDLine(1, "Rebooting...");
            playBootTone();
        } else {
            Update.printError(Serial);
            printLCDLine(0, "OTA Failed!");
            printLCDLine(1, "Error Flashing");
        }
    }
}
