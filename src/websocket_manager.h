#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <ArduinoWebsockets.h>

using namespace websockets;

class WebSocketManager {
	public:
    	WebSocketManager();  // Default constructor
		void connectToWebSocket();
		void pollWebSocket();
		void reconnectWebSocket();
	
	private:
		WebsocketsClient wsClient;  // The WebSocket client
};

#endif
