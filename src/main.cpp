#include "config.h"
#include "wifi_manager.h"
#include "websocket_manager.h"
#include "webserver_manager.h"

void setup() {
  Serial.begin(115200);
  delay(5000); // Ensure time for Serial Monitor to connect

  // Setup onboard LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Turn off LED initially

  // Initialize Wi-Fi manager and try to connect to saved Wi-Fi
  if (wifiManager.connectToStoredWiFi() == false) {
    // If connection fails, start the Access Point mode
    wifiManager.startAccessPoint();
  } else {
    // If connected, initialize WebSocket client
    websocketManager.connectToWebSocket();
  }
}

void loop() {
  websocketManager.pollWebSocket();
  // Handle client requests in Access Point mode
  webserverManager.handleClientRequests();
}
