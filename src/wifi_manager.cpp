#include "wifi_manager.h"
#include "config.h"
#include "webserver_manager.h"

Preferences preferences;

WiFiCredentials WiFiManager::getStoredWiFiCredentials() {
  WiFiCredentials creds;

  // Start the preferences with the namespace "wifi-creds"
  preferences.begin("wifi-creds", false);
  
  // Retrieve stored SSID and password
  creds.ssid = preferences.getString("ssid", "");
  creds.password = preferences.getString("password", "");
  
  // End preferences access
  preferences.end();

  return creds;  // Return the credentials struct
}

bool WiFiManager::connectToStoredWiFi() {
  WiFiCredentials creds = getStoredWiFiCredentials();

  Serial.println("In connect to stored wifi: stored_ssid: " + creds.ssid);
  Serial.println("In connect to stored wifi: stored_password: " + creds.password);
    
  if (creds.ssid == "" || creds.password == "") {
    Serial.println("No stored credentials found.");
    return false;
  }

  // Try to connect to the saved Wi-Fi credentials
  WiFi.begin(creds.ssid.c_str(), creds.password.c_str());
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

void WiFiManager::reconnectWiFi() {
    WiFiCredentials creds = getStoredWiFiCredentials();

    int retries = 0;
    WiFi.begin(creds.ssid.c_str(), creds.password.c_str());
    while (WiFi.status() != WL_CONNECTED && retries < 10) {
        delay(500);
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Reconnected to Wi-Fi!");
        digitalWrite(LED_PIN, HIGH);  // Turn on LED to indicate success
    } else {
        Serial.println("Failed to reconnect to Wi-Fi.");
    }
}

WiFiManager wifiManager;  // Create global instance
