#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <ArduinoWebsockets.h>

using namespace websockets;

class WebSocketManager {
	public:
		WebSocketManager();
		void connectToWebSocket();
		void pollWebSocket();
		void reconnectWebSocket();
	
	private:
		WebsocketsClient wsClient;
};

#endif
