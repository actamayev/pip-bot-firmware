#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <ArduinoWebsockets.h>

using namespace websockets;

class WebSocketManager {
public:
	void connectToWebSocket();
	void pollWebSocket();
	void reconnectWebSocket();
};

extern WebSocketManager websocketManager;
extern WebsocketsClient wsClient;

#endif
