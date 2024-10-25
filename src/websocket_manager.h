#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <ArduinoWebsockets.h>

using namespace websockets;

class WebSocketManager {
	private:
		WebsocketsClient wsClient;  // The WebSocket client

	public:
    	WebSocketManager();  // Default constructor
		void connectToWebSocket();
		void pollWebSocket();
		void reconnectWebSocket();
};

#endif
