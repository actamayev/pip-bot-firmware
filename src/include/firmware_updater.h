#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include <Update.h>
#include <Arduino.h>
#include <mbedtls/base64.h>

class FirmwareUpdater {
    private:
        static const size_t TRANSFER_BUFFER_SIZE = 128 * 1024;  // 128KB buffer in PSRAM
        static const size_t WORKING_BUFFER_SIZE = 16 * 1024;    // 16KB working buffer in RAM
        static const size_t HEAP_OVERHEAD = 32 * 1024;          // 32KB overhead

        uint8_t* transferBuffer;  // Large PSRAM buffer for receiving data
        uint8_t* workingBuffer;  // Smaller RAM buffer for processing

        // Update state tracking
        struct UpdateState {
            size_t totalSize;
            size_t receivedSize;
            size_t totalChunks;
            size_t receivedChunks;
            bool updateStarted;
            uint32_t lastChunkTime;
            size_t transferBufferPos;  // Current position in transfer buffer

            UpdateState() : totalSize(0), receivedSize(0), totalChunks(0),
                        receivedChunks(0), updateStarted(false), lastChunkTime(0),
                        transferBufferPos(0) {}
        };

        UpdateState state;

        // Private helper methods
        // bool decodeBase64(const char* input, uint8_t* output, size_t* outputLength);
        bool initializeBuffers();
        bool checkMemoryRequirements(size_t updateSize) const;
        void resetState();
        void processTransferBuffer();
        // bool canAcceptChunk(size_t chunkSize) const;

    public:
        FirmwareUpdater();
        ~FirmwareUpdater();

        // Core update functionality
        bool begin(size_t size);
        void processChunk(uint8_t* data, size_t dataLen, size_t chunkIndex, bool isLast);
        void end(bool success);
        void checkTimeout();

        // Status methods
        bool isUpdateInProgress() const { return state.updateStarted; }
        void setTotalChunks(size_t chunks) { state.totalChunks = chunks; }
};

#endif
