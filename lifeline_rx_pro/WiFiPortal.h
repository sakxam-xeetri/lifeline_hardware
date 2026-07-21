#ifndef WIFI_PORTAL_H
#define WIFI_PORTAL_H

#include "Config.h"
#include <WebServer.h>
#include <DNSServer.h>

extern WebServer wifiServer;
extern DNSServer dnsServer;

extern bool wifiConnected;
extern bool portalActive;
extern unsigned long portalStartTime;
extern unsigned long buttonPressStartTime;
extern bool buttonPressed;
extern int lastBeepSecond;

extern WiFiNetwork storedNetworks[MAX_WIFI_NETWORKS];
extern int networkCount;
extern String activeSSID;

void loadWiFiCredentials();
void saveWiFiCredentialsList(const WiFiNetwork nets[], int count);
bool connectToWiFi();
bool connectToWiFiSilent();
void startWiFiPortal();
void stopWiFiPortal();
void checkWiFiPortalButton();
void handleWiFiPortal();

#endif // WIFI_PORTAL_H
