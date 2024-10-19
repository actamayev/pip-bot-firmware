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
    // Periodically handle DNS and web server requests (non-blocking)
    webserverManager.handleClientRequests();

    // Periodically check WebSocket connection
    websocketManager.pollWebSocket();

    // Check if Wi-Fi is still connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Lost Wi-Fi connection. Reconnecting...");
        digitalWrite(LED_PIN, LOW);  // Turn off LED to indicate failure
        wifiManager.reconnectWiFi();
    }

    // Check if WebSocket is still connected, if not, try to reconnect
    if (!wsClient.available()) {
        Serial.println("WebSocket connection lost. Reconnecting...");
        websocketManager.reconnectWebSocket();
    }

    delay(100);  // Short delay to prevent tight looping
}
