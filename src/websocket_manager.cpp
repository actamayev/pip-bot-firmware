#include "websocket_manager.h"
#include "config.h"
#include <ArduinoWebsockets.h>
using namespace websockets;

WebsocketsClient wsClient;

void WebSocketManager::connectToWebSocket() {
  wsClient.onMessage([](WebsocketsMessage message) {
    Serial.print("Received message: ");
    Serial.println(message.data());
  });

  Serial.println("Connecting to WebSocket server...");
  wsClient.connect(ws_server_url);
  wsClient.send("Hello from ESP32!");
}

void WebSocketManager::pollWebSocket() {
  wsClient.poll();
}

WebSocketManager websocketManager;  // Create global instance
