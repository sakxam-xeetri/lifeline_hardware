#include "WiFiPortal.h"
#include "DisplayUI.h"
#include "BuzzerLED.h"
#include <WiFi.h>
#include <Preferences.h>

WebServer wifiServer(80);
DNSServer dnsServer;
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
            initNTPTime();
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
        Serial.println(F("[WIFI] No credentials stored"));
        wifiConnected = false;
        playWiFiFailTone();
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
            initNTPTime();
            Serial.printf("[WIFI] Connected to %s! IP: %s\n", activeSSID.c_str(), ip.c_str());
            
            currentScreen = SCREEN_WIFI_SPLASH;
            drawWiFiConnectedScreen(ip);
            playWiFiSuccessTone();
            delay(WIFI_SPLASH_TIME);
            return true;
        }
    }
    
    wifiConnected = false;
    Serial.println(F("[WIFI] All network connections failed"));
    playWiFiFailTone();
    return false;
}

void startWiFiPortal() {
    Serial.println(F("[WIFI] Starting Captive WiFi configuration portal..."));
    
    WiFi.disconnect(true);
    delay(100);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID);
    
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[WIFI] Open AP started! SSID: '%s'\n", WIFI_AP_SSID);
    Serial.printf("[WIFI] Portal IP: %s\n", apIP.toString().c_str());
    
    // Start DNS Server for captive portal auto-popup on port 53
    dnsServer.start(53, "*", apIP);
    
    wifiServer.on("/", handlePortalRoot);
    wifiServer.on("/save", HTTP_POST, handlePortalSave);
    
    // Captive portal detect endpoints -> auto pop up HTML
    wifiServer.on("/generate_204", handlePortalRoot);
    wifiServer.on("/redirect", handlePortalRoot);
    wifiServer.on("/hotspot-detect.html", handlePortalRoot);
    wifiServer.on("/canonical.html", handlePortalRoot);
    wifiServer.on("/nconnect.txt", handlePortalRoot);
    wifiServer.onNotFound([]() {
        wifiServer.sendHeader("Location", "http://192.168.4.1/", true);
        wifiServer.send(302, "text/plain", "");
    });
    
    wifiServer.begin();
    
    portalActive = true;
    portalStartTime = millis();
    currentScreen = SCREEN_PORTAL;
    
    drawWiFiPortalScreen();
}

void stopWiFiPortal() {
    if (!portalActive) return;
    
    Serial.println(F("[WIFI] Stopping portal..."));
    dnsServer.stop();
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
            previousScreenBeforePress = currentScreen;
        } else {
            unsigned long pressDuration = millis() - buttonPressStartTime;
            int secondsHeld = pressDuration / 1000;
            
            if (secondsHeld != lastBeepSecond && secondsHeld >= 1 && secondsHeld <= 3) {
                lastBeepSecond = secondsHeld;
                playCountdownTickTone();
            }
            
            if (!portalActive && pressDuration >= 150) {
                currentScreen = SCREEN_COUNTDOWN;
                drawProgressCountdownScreen(pressDuration, LONG_PRESS_DURATION);
            }
            
            if (pressDuration >= LONG_PRESS_DURATION) {
                buttonPressed = false;
                
                if (portalActive) {
                    stopWiFiPortal();
                } else {
                    playPortalOpenTone();
                    startWiFiPortal();
                }
            }
        }
    } else {
        if (buttonPressed) {
            buttonPressed = false;
            unsigned long totalDuration = millis() - buttonPressStartTime;
            lastBeepSecond = -1;
            
            if (!portalActive) {
                if (totalDuration < 1000) {
                    // Short press
                    if (previousScreenBeforePress == SCREEN_NO_WIFI) {
                        // Skip No WiFi screen
                        playSkipConfirmTone();
                        currentScreen = SCREEN_IDLE;
                        drawIdleScreen();
                    } else if (previousScreenBeforePress == SCREEN_IDLE) {
                        currentScreen = SCREEN_IDLE;
                        drawIdleScreen();
                    } else if (previousScreenBeforePress == SCREEN_ALERT) {
                        currentScreen = SCREEN_ALERT;
                        drawAlertScreen(lastDeviceId, lastAlertIndex, lastRssi);
                    }
                } else {
                    // Pressed between 1s and 3s, released before 3s -> cancel countdown
                    currentScreen = previousScreenBeforePress;
                    if (currentScreen == SCREEN_NO_WIFI) {
                        drawNoWiFiScreen();
                    } else if (currentScreen == SCREEN_IDLE) {
                        drawIdleScreen();
                    } else if (currentScreen == SCREEN_ALERT) {
                        drawAlertScreen(lastDeviceId, lastAlertIndex, lastRssi);
                    }
                }
            }
        }
    }
}

void handleWiFiPortal() {
    if (!portalActive) return;
    
    dnsServer.processNextRequest();
    wifiServer.handleClient();
    
    if (millis() - portalStartTime >= WIFI_PORTAL_TIMEOUT) {
        Serial.println(F("[WIFI] Portal timeout, closing..."));
        stopWiFiPortal();
    }
}

// HTML and Endpoint Handlers (Dark Theme, Sharp Edges, Crimson Red Aesthetic, Clean ASCII Text)
static String getPortalHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LIFELINE RX - CAPTIVE PORTAL</title>";
    html += "<style>";
    html += "* { box-sizing: border-box; border-radius: 0px !important; }";
    html += "body { font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #08080c; color: #f4f4f7; margin: 0; padding: 20px 15px; }";
    html += ".container { max-width: 440px; margin: 0 auto; background: #121218; padding: 25px 22px; border: 1px solid #ff1e42; box-shadow: 0 0 25px rgba(255, 30, 66, 0.15); }";
    html += ".header { border-bottom: 2px solid #ff1e42; padding-bottom: 12px; margin-bottom: 20px; text-align: left; }";
    html += "h1 { color: #ffffff; font-size: 22px; margin: 0 0 5px 0; font-weight: 800; letter-spacing: 1px; }";
    html += ".brand-sub { color: #ff1e42; font-size: 11px; font-weight: 700; letter-spacing: 1.5px; text-transform: uppercase; }";
    html += ".info-box { background: #1a1a24; border-left: 3px solid #ff1e42; padding: 10px 12px; margin-bottom: 22px; font-size: 12px; color: #b3b3c2; line-height: 1.4; }";
    html += "h2 { color: #ffffff; font-size: 13px; font-weight: 700; margin: 18px 0 8px 0; text-transform: uppercase; letter-spacing: 1px; border-left: 2px solid #ff1e42; padding-left: 8px; }";
    html += "label { display: block; font-size: 11px; color: #8c8c9e; text-transform: uppercase; font-weight: 700; margin-top: 8px; margin-bottom: 4px; letter-spacing: 0.5px; }";
    html += "input[type=text], input[type=password] { width: 100%; padding: 12px; margin-bottom: 12px; border: 1px solid #282836; background: #0a0a0f; color: #ffffff; font-size: 14px; font-family: monospace; outline: none; transition: border-color 0.2s, box-shadow 0.2s; }";
    html += "input[type=text]:focus, input[type=password]:focus { border-color: #ff1e42; box-shadow: 0 0 10px rgba(255, 30, 66, 0.4); }";
    html += "input[type=submit] { width: 100%; padding: 14px; background: #ff1e42; color: #ffffff; border: 1px solid #ff1e42; cursor: pointer; font-weight: 800; font-size: 14px; text-transform: uppercase; letter-spacing: 1px; margin-top: 15px; transition: background 0.2s, box-shadow 0.2s; box-shadow: 0 0 12px rgba(255, 30, 66, 0.3); }";
    html += "input[type=submit]:hover { background: #e01235; box-shadow: 0 0 20px rgba(255, 30, 66, 0.6); }";
    html += ".status { text-align: center; margin-top: 20px; padding: 8px; font-size: 11px; background: #0a0a0f; border: 1px solid #282836; color: #727285; letter-spacing: 0.5px; }";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>LIFELINE RX</h1>";
    html += "<div class='brand-sub'>Base Station - Captive WiFi Setup</div>";
    html += "</div>";
    html += "<div class='info-box'>Configure 1, 2, or 3 WiFi networks. Network 1 is primary. Leave Network 2 & 3 blank if using only 1 WiFi network.</div>";
    html += "<form action='/save' method='POST'>";
    
    for (int i = 0; i < 3; i++) {
        String numStr = String(i + 1);
        String currentS = (i < networkCount) ? storedNetworks[i].ssid : "";
        String currentP = (i < networkCount) ? storedNetworks[i].password : "";
        
        html += "<h2>WiFi Network #" + numStr + (i == 0 ? " (Primary Required)" : " (Optional Backup)") + "</h2>";
        html += "<label>SSID (Network Name):</label>";
        html += "<input type='text' name='ssid" + numStr + "' placeholder='Network SSID' value='" + currentS + "'" + (i == 0 ? " required" : "") + ">";
        html += "<label>WPA2 Password:</label>";
        html += "<input type='password' name='pass" + numStr + "' placeholder='WiFi Password' value='" + currentP + "'>";
    }
    
    html += "<input type='submit' value='SAVE & CONNECT WI-FI'>";
    html += "</form>";
    
    html += "<div class='status'>STORED NETWORKS: " + String(networkCount) + " / 3</div>";
    
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
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LIFELINE RX - SAVED</title>";
    html += "<style>* { box-sizing: border-box; border-radius: 0px !important; } body{font-family:'Segoe UI',sans-serif;background:#08080c;color:#fff;text-align:center;padding:50px 15px;}";
    html += ".card{background:#121218;border:1px solid #ff1e42;padding:30px 20px;max-width:420px;margin:0 auto;box-shadow:0 0 25px rgba(255,30,66,0.2);}";
    html += "h2{color:#ff1e42;font-size:20px;margin-bottom:10px;text-transform:uppercase;letter-spacing:1px;} p{color:#b3b3c2;font-size:13px;}</style></head><body>";
    html += "<div class='card'><h2>CONFIG SAVED</h2>";
    html += "<p>Successfully saved " + String(count) + " network(s).</p>";
    html += "<p>Device is restarting and connecting...</p></div>";
    html += "</body></html>";
    
    wifiServer.send(200, "text/html", html);
    
    delay(2000);
    ESP.restart();
}
