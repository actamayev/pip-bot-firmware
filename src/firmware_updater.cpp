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

// bool FirmwareUpdater::decodeBase64(const char* input, uint8_t* output, size_t* outputLength) {
//     size_t inputLength = strlen(input);
//     size_t written = 0;

//     int result = mbedtls_base64_decode(
//         output,
//         *outputLength,
//         &written,
//         (const unsigned char*)input,
//         inputLength
//     );

//     if (result == 0) {
//         *outputLength = written;
//         return true;
//     }
//     return false;
// }

// bool FirmwareUpdater::canAcceptChunk(size_t chunkSize) const {
//     // Check if there's room in the transfer buffer
//     size_t availableSpace = TRANSFER_BUFFER_SIZE - state.transferBufferPos;
//     // Account for base64 decoding (output is about 3/4 of input)
//     size_t decodedSize = (chunkSize * 3) / 4;
//     return decodedSize <= availableSpace;
// }

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
    if (!state.updateStarted) {
        Serial.println("Update not properly initialized");
        return;
    }

    Serial.printf("Writing chunk %d, size: %d bytes\n", chunkIndex, dataLen);

    if (Update.write(data, dataLen) != dataLen) {
        Serial.printf("Write failed at chunk: %u\n", chunkIndex);
        end(false);
        return;
    }

    state.receivedSize += dataLen;
    state.receivedChunks++;
    state.lastChunkTime = millis();

    Serial.printf("Progress: %d%%\n", (state.receivedSize * 100) / state.totalSize);

    if (isLast) {
        Serial.println("Processing final chunk");
        end(true);
    }
    // if (!transferBuffer || !workingBuffer || !state.updateStarted) {
    //     Serial.println("Update not properly initialized");
    //     return;
    // }

    // // Check if we need to process current buffer first
    // if (!canAcceptChunk(strlen(data))) {
    //     Serial.println("Processing current buffer before accepting new chunk");
    //     processTransferBuffer();
    // }

    // // Decode base64 directly into transfer buffer at current position
    // size_t decodedLen = TRANSFER_BUFFER_SIZE - state.transferBufferPos;
    // if (!decodeBase64(data, transferBuffer + state.transferBufferPos, &decodedLen)) {
    //     Serial.println("Base64 decode failed");
    //     end(false);
    //     return;
    // }

    // state.transferBufferPos += decodedLen;
    // state.receivedSize += decodedLen;
    // state.receivedChunks++;
    // state.lastChunkTime = millis();

    // // Process buffer if it's getting full or this is the last chunk
    // if (isLast || state.transferBufferPos >= (TRANSFER_BUFFER_SIZE * 0.8)) {
    //     processTransferBuffer();
    // }

    // if (isLast) {
    //     Serial.println("Processing final chunk");
    //     end(true);
    // }
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
