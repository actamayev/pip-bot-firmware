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

		// Fixed buffer for decoding
		static const size_t BUFFER_SIZE = 8192; // 8KB buffer
		uint8_t* buffer = nullptr;

		// Update state tracking
		struct UpdateState {
			size_t totalSize;
			size_t receivedSize;
			size_t totalChunks;
			size_t receivedChunks;
			bool updateStarted;
			uint32_t lastChunkTime;

			UpdateState() : totalSize(0), receivedSize(0), totalChunks(0),
						receivedChunks(0), updateStarted(false), lastChunkTime(0) {}
		};

		UpdateState updateState;

		// Private methods
		bool decodeBase64(const char* input, uint8_t* output, size_t* outputLength);
		void handleMessage(websockets::WebsocketsMessage message);
		bool startUpdate(size_t size);
		void endUpdate(bool success);
		void processChunk(const char* data, size_t chunkIndex, bool isLast);

	public:
		WebSocketManager();
		~WebSocketManager();
		void connectToWebSocket();
		void pollWebSocket();
		void reconnectWebSocket();
		void resetUpdateState() {
			updateState = UpdateState();
		}
};

#endif
