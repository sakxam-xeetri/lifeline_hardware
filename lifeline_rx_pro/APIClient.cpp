#include "APIClient.h"
#include <WiFi.h>
#include <HTTPClient.h>

extern bool wifiConnected;

void pushAlertToAPI(int deviceId, int alertIndex, int rssi) {
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[API] WiFi not connected, skipping API push"));
        return;
    }
    
    HTTPClient http;
    http.begin(API_ENDPOINT);
    http.addHeader("Content-Type", "application/json");
    
    String jsonPayload = "{\"DID\":" + String(deviceId) + 
                         ",\"message_code\":" + String(alertIndex) + 
                         ",\"RSSI\":" + String(rssi) + "}";
    
    Serial.printf("[API] Sending: %s\n", jsonPayload.c_str());
    
    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("[API] Response (%d): %s\n", httpResponseCode, response.c_str());
    } else {
        Serial.printf("[API] Error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
}
