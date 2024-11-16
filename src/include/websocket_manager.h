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
		void handleBinaryUpdate(const uint8_t* data, size_t len, bool isLastChunk);
		void handleMessage(websockets::WebsocketsMessage message);
    	bool decodeBase64(const char* input, uint8_t* output, size_t* outputLength);
		struct UpdateState {
			size_t totalSize;
			size_t receivedSize;
			size_t totalChunks;
			size_t receivedChunks;
			bool updateStarted;
        	UpdateState() : totalSize(0), receivedSize(0), totalChunks(0), 
                       receivedChunks(0), updateStarted(false) {}
    	};

    	UpdateState updateState;

	public:
		WebSocketManager();
		void connectToWebSocket();
		void pollWebSocket();
		void reconnectWebSocket();
		void resetUpdateState() {
        	updateState = UpdateState();
		}
};

#endif
