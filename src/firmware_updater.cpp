#include "./include/firmware_updater.h"

FirmwareUpdater::FirmwareUpdater() : transferBuffer(nullptr), workingBuffer(nullptr) {
    initializeBuffers();
}

FirmwareUpdater::~FirmwareUpdater() {
    if (transferBuffer != nullptr) {
        free(transferBuffer);
        transferBuffer = nullptr;
    }
    if (workingBuffer != nullptr) {
        free(workingBuffer);
        workingBuffer = nullptr;
    }
}

bool FirmwareUpdater::initializeBuffers() {
    // Allocate large buffer in PSRAM
    if (transferBuffer == nullptr) {
        transferBuffer = (uint8_t*)ps_malloc(TRANSFER_BUFFER_SIZE);
        if (transferBuffer == nullptr) {
            Serial.println("Failed to allocate PSRAM transfer buffer");
            return false;
        }
        Serial.printf("Allocated %u byte PSRAM transfer buffer\n", TRANSFER_BUFFER_SIZE);
    }

    // Allocate smaller buffer in RAM
    if (workingBuffer == nullptr) {
        workingBuffer = (uint8_t*)malloc(WORKING_BUFFER_SIZE);
        if (workingBuffer == nullptr) {
            Serial.println("Failed to allocate RAM working buffer");
            return false;
        }
        Serial.printf("Allocated %u byte RAM working buffer\n", WORKING_BUFFER_SIZE);
    }

    return true;
}

bool FirmwareUpdater::checkMemoryRequirements(size_t updateSize) const {
    const size_t requiredHeap = WORKING_BUFFER_SIZE + HEAP_OVERHEAD;
    const size_t freeHeap = ESP.getFreeHeap();
    const size_t freePsram = ESP.getFreePsram();

    Serial.printf("\nMemory Requirements:\n");
    Serial.printf("- Required heap: %u bytes\n", requiredHeap);
    Serial.printf("- Required PSRAM: %u bytes\n", TRANSFER_BUFFER_SIZE);
    Serial.printf("- Update size: %u bytes\n", updateSize);

    if (freeHeap < requiredHeap) {
        Serial.printf("Insufficient heap. Need %u more bytes\n", 
            requiredHeap - freeHeap);
        return false;
    }

    if (freePsram < TRANSFER_BUFFER_SIZE) {
        Serial.printf("Insufficient PSRAM. Need %u more bytes\n",
            TRANSFER_BUFFER_SIZE - freePsram);
        return false;
    }

    return true;
}

void FirmwareUpdater::processTransferBuffer() {
    if (state.transferBufferPos == 0) return;

    size_t remaining = state.transferBufferPos;
    size_t processedBytes = 0;

    while (remaining > 0) {
        size_t chunkSize = min(remaining, WORKING_BUFFER_SIZE);
        
        // Copy chunk from PSRAM to RAM working buffer
        memcpy(workingBuffer, transferBuffer + processedBytes, chunkSize);

        // Write to flash
        if (Update.write(workingBuffer, chunkSize) != chunkSize) {
            Serial.printf("Write failed at offset: %u\n", processedBytes);
            end(false);
            return;
        }

        processedBytes += chunkSize;
        remaining -= chunkSize;
    }

    // Reset transfer buffer position after processing
    state.transferBufferPos = 0;
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

    state.updateStarted = true;
    state.totalSize = size;
    state.receivedSize = 0;
    state.transferBufferPos = 0;
    state.lastChunkTime = millis();

    Serial.printf("Update started. Size: %u bytes\n", size);
    return true;
}

void FirmwareUpdater::processChunk(uint8_t* data, size_t dataLen, size_t chunkIndex, bool isLast) {
    unsigned long startTime = millis();
    
    if (!state.updateStarted) {
        Serial.println("Update not properly initialized");
        return;
    }

    Serial.printf("Writing chunk %d, size: %d bytes\n", chunkIndex, dataLen);

    // Write to flash
    unsigned long writeStart = millis();
    if (Update.write(data, dataLen) != dataLen) {
        Serial.printf("Write failed at chunk: %u\n", chunkIndex);
        end(false);
        return;
    }
    Serial.printf("Flash write time: %lu ms\n", millis() - writeStart);

    state.receivedSize += dataLen;
    state.receivedChunks++;
    state.lastChunkTime = millis();

    Serial.printf("Progress: %d%%\n", (state.receivedSize * 100) / state.totalSize);

    if (isLast) {
        Serial.println("Processing final chunk");
        unsigned long endStart = millis();
        end(true);
        Serial.printf("End processing time: %lu ms\n", millis() - endStart);
    }

    Serial.printf("Total chunk processing time: %lu ms\n", millis() - startTime);
}

void FirmwareUpdater::end(bool success) {
    if (!state.updateStarted) return;

    // Process any remaining data in transfer buffer
    if (state.transferBufferPos > 0) {
        processTransferBuffer();
    }

    if (success && Update.end()) {
        Serial.println("Update successful!");
        delay(1000);
        ESP.restart();
    } else {
        Serial.printf("Update failed: %s\n", Update.errorString());
        Update.abort();
    }
    resetState();
}

void FirmwareUpdater::checkTimeout() {
    if (state.updateStarted && 
        (millis() - state.lastChunkTime > 15000)) { // 15 second timeout
        Serial.println("Update timeout");
        end(false);
    }
}

void FirmwareUpdater::resetState() {
    state = UpdateState();
}
