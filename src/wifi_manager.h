#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>

class WiFiManager {
public:
  bool connectToStoredWiFi();
  void startAccessPoint();
};

extern WiFiManager wifiManager;
extern Preferences preferences;

#endif
