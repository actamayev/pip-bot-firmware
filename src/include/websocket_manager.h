#ifndef WEBSOCKET_MANAGER_H
#define WEBSOCKET_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#define JSMN_HEADER
#include "./jsmn.h"
#include "./firmware_updater.h"

using namespace websockets;

class WebSocketManager {
	private:
        static const size_t MAX_TOKENS = 32;  // Maximum number of JSON tokens
        jsmn_parser parser;
        jsmntok_t tokens[MAX_TOKENS];

        static const size_t SMALL_DOC_SIZE = 256;       // For small outgoing messages

        websockets::WebsocketsClient wsClient;
        FirmwareUpdater updater;

        void handleMessage(websockets::WebsocketsMessage message);
		void sendErrorMessage(const char* error);
        int64_t extractInt(const char* json, const jsmntok_t* tok);
        bool extractBool(const char* json, const jsmntok_t* tok);
        void sendJsonMessage(const char* event, const char* status, const char* extra = nullptr);
        void processChunk(const char* chunkData, int chunkDataLength, size_t chunkIndex, size_t totalChunks, size_t totalSize, bool isLast);
	public:
		WebSocketManager();
		void connectToWebSocket();
		void pollWebSocket();
};

#endif
