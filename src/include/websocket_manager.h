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
        static WebSocketManager* instance;

        struct ChunkMetadata {
            size_t chunkIndex;
            size_t totalChunks;
            size_t totalSize;
            size_t chunkSize;  // Add this
            bool isLast;
            bool expectingBinary;
            
            ChunkMetadata() : chunkIndex(0), totalChunks(0), totalSize(0), 
                chunkSize(0), isLast(false), expectingBinary(false) {}
        };
        ChunkMetadata currentChunk;

        static const size_t MAX_TOKENS = 32;  // Maximum number of JSON tokens
        jsmn_parser parser;
        jsmntok_t tokens[MAX_TOKENS];

        static const size_t SMALL_DOC_SIZE = 256;       // For small outgoing messages

        websockets::WebsocketsClient wsClient;
        FirmwareUpdater updater;

        // Make constructor private for singleton
        WebSocketManager();

        void handleMessage(websockets::WebsocketsMessage message);
		// void sendErrorMessage(const char* error);
        int64_t extractInt(const char* json, const jsmntok_t* tok);
        bool extractBool(const char* json, const jsmntok_t* tok);
        // void sendJsonMessage(const char* event, const char* status, const char* extra = nullptr);
        void processChunk(uint8_t* chunkData, size_t chunkDataLength, size_t chunkIndex, size_t totalChunks, size_t totalSize, bool isLast);
        void handleJsonMessage(WebsocketsMessage message);
        void handleBinaryMessage(WebsocketsMessage message);

        WebSocketManager(const WebSocketManager&) = delete;
        WebSocketManager& operator=(const WebSocketManager&) = delete;
	public:
		// Singleton access
        static WebSocketManager& getInstance() {
            if (instance == nullptr) {
                instance = new WebSocketManager();
            }
            return *instance;
        }

        void connectToWebSocket();
        void pollWebSocket();
        
        // Make these public so they can be called from FirmwareUpdater
        void sendErrorMessage(const char* error);
        void sendJsonMessage(const char* event, const char* status, const char* extra = nullptr);
};

#endif
