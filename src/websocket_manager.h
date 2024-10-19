#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <ArduinoWebsockets.h>

class WebSocketManager {
public:
  void connectToWebSocket();
  void pollWebSocket();
};

extern WebSocketManager websocketManager;

#endif
