#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Update.h>
#include <ArduinoJson.h>
#include <mbedtls/base64.h>
#include <ArduinoWebsockets.h>

using namespace websockets;

class WebSocketManager {
	private:
		websockets::WebsocketsClient wsClient;
		void handleBinaryUpdate(const uint8_t* data, size_t len);
		void handleMessage(websockets::WebsocketsMessage message);
    	bool decodeBase64(const char* input, uint8_t* output, size_t* outputLength);

	public:
		WebSocketManager();
		void connectToWebSocket();
		void pollWebSocket();
		void reconnectWebSocket();
};

#endif
