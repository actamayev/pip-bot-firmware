#include "wifi_manager.h"
#include "config.h"
#include "webserver_manager.h"

Preferences preferences;

bool WiFiManager::connectToStoredWiFi() {
  // Load Wi-Fi credentials from memory
  preferences.begin("wifi-creds", false);
  String stored_ssid = preferences.getString("ssid", "");
  String stored_password = preferences.getString("password", "");
  preferences.end();

  Serial.println("In connect to stored wifi: stored_ssid: " + stored_ssid);
  Serial.println("In connect to stored wifi: stored_password: " + stored_password);
    
  if (stored_ssid == "" || stored_password == "") {
    Serial.println("No stored credentials found.");
    return false;
  }

  // Try to connect to the saved Wi-Fi credentials
  WiFi.begin(stored_ssid.c_str(), stored_password.c_str());
  Serial.println("Attempting to connect to saved Wi-Fi...");

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi!");
    digitalWrite(LED_PIN, HIGH);  // Turn on LED to show success
    return true;
  } else {
    Serial.println("Failed to connect to saved Wi-Fi.");
    return false;
  }
}

void WiFiManager::startAccessPoint() {
  // Start AP mode and the web server
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  webserverManager.startWebServer();
}

WiFiManager wifiManager;  // Create global instance
