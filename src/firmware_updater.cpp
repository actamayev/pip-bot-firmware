#include "./include/firmware_updater.h"
#include "./include/websocket_manager.h"  // Include this after the forward declaration

FirmwareUpdater::FirmwareUpdater() 
    : writeBuffer(nullptr), receiveBuffer(nullptr),
      updateRunning(false), bufferReady(false), currentChunkSize(0),
      flashWriteTask(nullptr)
{
    initializeBuffers();
}

FirmwareUpdater::~FirmwareUpdater() {
    updateRunning = false;
    if (flashWriteTask) {
        vTaskDelete(flashWriteTask);
        flashWriteTask = nullptr;
    }
    if (writeBuffer) {
        free(writeBuffer);
        writeBuffer = nullptr;
    }
    if (receiveBuffer) {
        free(receiveBuffer);
        receiveBuffer = nullptr;
    }
    if (bufferMutex) {
        vSemaphoreDelete(bufferMutex);
        bufferMutex = nullptr;
    }
}

bool FirmwareUpdater::initializeBuffers() {
    // Allocate double buffers in PSRAM for parallel processing
    if (writeBuffer == nullptr) {
        writeBuffer = (uint8_t*)ps_malloc(WORKING_BUFFER_SIZE);
        if (writeBuffer == nullptr) {
            Serial.println("Failed to allocate write buffer");
            return false;
        }
    }

    if (receiveBuffer == nullptr) {
        receiveBuffer = (uint8_t*)ps_malloc(WORKING_BUFFER_SIZE);
        if (receiveBuffer == nullptr) {
            Serial.println("Failed to allocate receive buffer");
            return false;
        }
    }

    bufferMutex = xSemaphoreCreateMutex();
    if (bufferMutex == nullptr) {
        Serial.println("Failed to create mutex");
        return false;
    }

    Serial.printf("Allocated dual %u byte buffers for parallel processing\n", WORKING_BUFFER_SIZE);
    return true;
}

void FirmwareUpdater::flashWriteTaskFunction(void* parameter) {
    FirmwareUpdater* updater = static_cast<FirmwareUpdater*>(parameter);
    updater->flashWriteLoop();
}

void FirmwareUpdater::flashWriteLoop() {
    while (updateRunning) {
        if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (bufferReady) {
                unsigned long writeStart = millis();

                if (Update.write(writeBuffer, currentChunkSize) != currentChunkSize) {
                    Serial.println("Flash write failed");
                    updateRunning = false;
                } else {
                    state.receivedSize += currentChunkSize;
                    Serial.printf("Progress: %d%%\n", (state.receivedSize * 100) / state.totalSize);
                }

                bufferReady = false;
                Serial.printf("Flash write took: %lu ms\n", millis() - writeStart);
            }
            xSemaphoreGive(bufferMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    flashWriteTask = nullptr;
    vTaskDelete(NULL);
}

bool FirmwareUpdater::begin(size_t size) {
    if (size == 0) return false;

    if (!checkMemoryRequirements(size) || !initializeBuffers()) {
        return false;
    }

    Serial.printf("Starting update of %u bytes\n", size);
    if (!Update.begin(size)) {
        Serial.printf("Not enough space: %s\n", Update.errorString());
        return false;
    }

    state.totalSize = size;
    state.receivedSize = 0;
    state.lastChunkTime = millis();
    updateRunning = true;
    bufferReady = false;

   BaseType_t result = xTaskCreatePinnedToCore(
        flashWriteTaskFunction,
        "FlashWrite",
        8192,
        this,
        1,
        &flashWriteTask,
        0
    );

    if (result != pdPASS) {
        Serial.println("Failed to create flash write task");
        return false;
    }

    Serial.printf("Update started. Size: %u bytes\n", size);
    return true;
}

bool FirmwareUpdater::checkMemoryRequirements(size_t updateSize) const {
    const size_t requiredHeap = WORKING_BUFFER_SIZE + HEAP_OVERHEAD;
    const size_t freeHeap = ESP.getFreeHeap();
    const size_t freePsram = ESP.getFreePsram();

    Serial.printf("\nMemory Requirements:\n");
    Serial.printf("- Required heap: %u bytes\n", requiredHeap);
    Serial.printf("- Update size: %u bytes\n", updateSize);

    if (freeHeap < requiredHeap) {
        Serial.printf("Insufficient heap. Need %u more bytes\n", 
            requiredHeap - freeHeap);
        return false;
    }

    return true;
}

void FirmwareUpdater::processChunk(uint8_t* data, size_t dataLen, size_t chunkIndex, bool isLast) {
    unsigned long startTime = millis();
    
    if (!updateRunning) {
        Serial.println("Update not properly initialized");
        return;
    }

    Serial.printf("Processing chunk %d, total size: %d bytes\n", chunkIndex, dataLen);

    // Process large chunks in smaller pieces
    size_t remaining = dataLen;
    size_t offset = 0;
    
    while (remaining > 0) {
        size_t pieceSize = min(remaining, WORKING_BUFFER_SIZE);
        
        // Copy piece to receive buffer
        memcpy(receiveBuffer, data + offset, pieceSize);
        
        // Wait for previous write to complete
        while (bufferReady) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        // Swap buffers and trigger write
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE) {
            uint8_t* temp = writeBuffer;
            writeBuffer = receiveBuffer;
            receiveBuffer = temp;
            
            currentChunkSize = pieceSize;
            bufferReady = true;
            state.lastChunkTime = millis();
            
            xSemaphoreGive(bufferMutex);
        }

        // Wait for this piece to be written before processing next piece
        while (bufferReady) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        remaining -= pieceSize;
        offset += pieceSize;
        
        Serial.printf("Processed %d/%d bytes\n", offset, dataLen);
    }

    state.receivedChunks++;

    if (isLast) {
        Serial.println("Processing final chunk");
        end(true);
    }

    Serial.printf("Total chunk processing time: %lu ms\n", millis() - startTime);
}

void FirmwareUpdater::end(bool success) {
    updateRunning = false;
    
    // Wait for flash task to complete
    while (flashWriteTask != nullptr) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (success && Update.end()) {
        Serial.println("Update successful!");
        WebSocketManager::getInstance().sendJsonMessage("update_status", "complete");
        delay(1000);
        ESP.restart();
    } else {
        Serial.printf("Update failed: %s\n", Update.errorString());
        Update.abort();
        WebSocketManager::getInstance().sendJsonMessage("update_status", "failed", Update.errorString());
    }
    resetState();
}

void FirmwareUpdater::checkTimeout() {
    if (updateRunning && 
        (millis() - state.lastChunkTime > 15000)) { // 15 second timeout
        Serial.println("Update timeout");
        end(false);
    }
}

void FirmwareUpdater::resetState() {
    state = UpdateState();
    updateRunning = false;
    bufferReady = false;
}
