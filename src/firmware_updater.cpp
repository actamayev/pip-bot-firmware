#include "./include/firmware_updater.h"

FirmwareUpdater::FirmwareUpdater(): buffer(nullptr) {
    if (!checkHeapSpace()) return;
    buffer = (uint8_t*)ps_malloc(BUFFER_SIZE);
    if (buffer == nullptr) {
        Serial.println("Failed to allocate update buffer");
    } else {
        Serial.printf("Successfully allocated %u byte buffer\n", BUFFER_SIZE);
    }
}

FirmwareUpdater::~FirmwareUpdater() {
    if (buffer != nullptr) {
        free(buffer);
        buffer = nullptr;
    }
}

bool FirmwareUpdater::checkHeapSpace() const {
    const size_t REQUIRED_HEAP = BUFFER_SIZE + HEAP_OVERHEAD;
    size_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < REQUIRED_HEAP) {
        Serial.printf("Insufficient heap space. Required: %u, Available: %u\n", 
            REQUIRED_HEAP, freeHeap);
        return false;
    }
    return true;
}

bool FirmwareUpdater::checkMemoryRequirements(size_t updateSize) const {
    const size_t requiredHeap = BUFFER_SIZE + HEAP_OVERHEAD;
    const size_t freeHeap = ESP.getFreeHeap();

    Serial.printf("Memory Check:\n");
    Serial.printf("- Required heap: %u bytes\n", requiredHeap);
    Serial.printf("- Free heap: %u bytes\n", freeHeap);
    Serial.printf("- Update size: %u bytes\n", updateSize);

    if (freeHeap < requiredHeap) {
        Serial.printf("Insufficient heap. Need %u more bytes\n", 
            requiredHeap - freeHeap);
        return false;
    }
    return true;
}

bool FirmwareUpdater::initializeBuffer() {
    if (buffer != nullptr) return true;
    
    buffer = (uint8_t*)ps_malloc(BUFFER_SIZE);
    if (buffer == nullptr) {
        Serial.println("Failed to allocate buffer");
        return false;
    }
    
    Serial.printf("Buffer allocated: %u bytes\n", BUFFER_SIZE);
    return true;
}

bool FirmwareUpdater::decodeBase64(const char* input, uint8_t* output, size_t* outputLength) {
    size_t inputLength = strlen(input);
    size_t written = 0;

    int result = mbedtls_base64_decode(
        output,
        *outputLength,
        &written,
        (const unsigned char*)input,
        inputLength
    );

    if (result == 0) {
        *outputLength = written;
        return true;
    }
    return false;
}

bool FirmwareUpdater::begin(size_t size) {
    if (
        size == 0 ||
        !checkMemoryRequirements(size) ||
        !initializeBuffer()
    ) return false;

    Serial.printf("Starting update of %u bytes\n", size);
    if (!Update.begin(size)) {
        Serial.printf("Not enough space: %s\n", Update.errorString());
        return false;
    }

    state.updateStarted = true;
    state.totalSize = size;
    state.receivedSize = 0;
    state.lastChunkTime = millis();

    Serial.printf("Update started. Size: %u bytes\n", size);
    return true;
}

void FirmwareUpdater::processChunk(const char* data, size_t chunkIndex, bool isLast) {
    if (!buffer || !state.updateStarted) {
        Serial.println("Update not properly initialized");
        return;
    }

    size_t decodedLen = BUFFER_SIZE;
    if (!decodeBase64(data, buffer, &decodedLen)) {
        Serial.println("Base64 decode failed");
        end(false);
        return;
    }

    if (Update.write(buffer, decodedLen) != decodedLen) {
        Serial.printf("Write failed at chunk: %u\n", chunkIndex);
        end(false);
        return;
    }

    state.receivedSize += decodedLen;
    state.receivedChunks++;
    state.lastChunkTime = millis();
    
    Serial.printf("Chunk %u/%u - Progress: %d%%\n", 
        chunkIndex + 1, 
        state.totalChunks,
        (state.receivedSize * 100) / state.totalSize
    );

    if (isLast) {
        Serial.println("Processing final chunk");
        end(true);
    }
}

void FirmwareUpdater::end(bool success) {
    if (!state.updateStarted) return;

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
        (millis() - state.lastChunkTime > 10000)) { // 10 second timeout
        Serial.println("Update timeout");
        end(false);
    }
}
