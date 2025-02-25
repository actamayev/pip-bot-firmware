#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#define JSMN_HEADER
#include "./jsmn.h"
#include "./singleton.h"
#include "./firmware_updater.h"
#include "./lab_demo_manager.h"

using namespace websockets;

class WebSocketManager : public Singleton<WebSocketManager> {
    friend class Singleton<WebSocketManager>;

    public:
        void connectToWebSocket();
        void pollWebSocket();

        void sendErrorMessage(const char* error);
        void sendJsonMessage(const char* event, const char* status, const char* extra = nullptr);

    private:
        struct ChunkMetadata {
            size_t chunkIndex;
            size_t totalChunks;
            size_t totalSize;
            size_t chunkSize;
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
        int64_t extractInt(const char* json, const jsmntok_t* tok);
        bool extractBool(const char* json, const jsmntok_t* tok);
        void processChunk(uint8_t* chunkData, size_t chunkDataLength, size_t chunkIndex, size_t totalChunks, size_t totalSize, bool isLast);
        void handleJsonMessage(WebsocketsMessage message);
        void handleFirmwareMetadata(const char* json, int tokenCount);
        void handleBinaryMessage(WebsocketsMessage message);
        bool attemptConnection();
        static const int MAX_RETRIES = 8;
        int retryCount;
        unsigned long startAttemptTime;
        bool connected;
        void sendInitialData();
};
