#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include <Update.h>
#include <Arduino.h>
#include <mbedtls/base64.h>

class FirmwareUpdater {
    private:
        static const size_t BUFFER_SIZE = 64 * 1024;       // 64KB buffer
        static const size_t HEAP_OVERHEAD = 32 * 1024;     // 32KB overhead

        uint8_t* buffer;
        
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

        UpdateState state;
        
        // Private helper methods
        bool decodeBase64(const char* input, uint8_t* output, size_t* outputLength);
        bool checkHeapSpace() const;
        bool initializeBuffer();
        bool checkMemoryRequirements(size_t updateSize) const;
        void resetState() { state = UpdateState(); }

    public:
        FirmwareUpdater();
        ~FirmwareUpdater();

        // Core update functionality
        bool begin(size_t size);
        void processChunk(const char* data, size_t chunkIndex, bool isLast);
        void end(bool success);
        void checkTimeout();
        
        // Status methods
        bool isUpdateInProgress() const { return state.updateStarted; }
        size_t getReceivedSize() const { return state.receivedSize; }
        size_t getTotalSize() const { return state.totalSize; }
        size_t getReceivedChunks() const { return state.receivedChunks; }
        size_t getTotalChunks() const { return state.totalChunks; }
        uint32_t getLastChunkTime() const { return state.lastChunkTime; }
        void setTotalChunks(size_t chunks) { state.totalChunks = chunks; }
};

#endif
