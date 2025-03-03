#pragma once

#include <Update.h>
#include <mbedtls/base64.h>
#include "./singleton.h"

class FirmwareUpdater : public Singleton<FirmwareUpdater> {
    friend class Singleton<FirmwareUpdater>;

    public:
        FirmwareUpdater();
        ~FirmwareUpdater();
        bool begin(size_t size);
        void processChunk(uint8_t* data, size_t dataLen, size_t chunkIndex, bool isLast);
        void end(bool success);
        void checkTimeout();
        bool isUpdateInProgress() const { return updateRunning; }
        void setTotalChunks(size_t chunks) { state.totalChunks = chunks; }

    private:
        static const size_t WORKING_BUFFER_SIZE = 64 * 1024;    // 64KB working buffer
        static const size_t HEAP_OVERHEAD = 32 * 1024;          // 32KB overhead

        // Parallel processing members
        TaskHandle_t flashWriteTask;
        SemaphoreHandle_t bufferMutex;
        volatile bool updateRunning;
        volatile bool bufferReady;

        uint8_t* writeBuffer;    // Buffer being written to flash
        uint8_t* receiveBuffer;  // Buffer for receiving new data
        size_t currentChunkSize;

        // State tracking
        struct UpdateState {
            size_t totalSize;
            size_t receivedSize;
            size_t totalChunks;
            size_t receivedChunks;
            uint32_t lastChunkTime;

            UpdateState() : totalSize(0), receivedSize(0), totalChunks(0),
                receivedChunks(0), lastChunkTime(0) {}
        };

        UpdateState state;

        // Static task function
        static void flashWriteTaskFunction(void* parameter);
        void flashWriteLoop();

        bool initializeBuffers();
        bool checkMemoryRequirements(size_t updateSize) const;
        void resetState();
};
