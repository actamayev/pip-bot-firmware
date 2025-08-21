#include "firmware_version_tracker.h"

FirmwareVersionTracker::FirmwareVersionTracker() {
    firmwareVersion = PreferencesManager::getInstance().getFirmwareVersion(); // used to set the class' firmwareVersion
    
    // Configure HTTPUpdate instance
    httpUpdate.onProgress([this](int curr, int total) {
        this->updateProgressLeds(curr, total);
    });
    
    // Set onEnd callback to update firmware version before reboot
    httpUpdate.onEnd([this]() {
        PreferencesManager::getInstance().setFirmwareVersion(this->pendingVersion);
    });

    // Setup clients based on environment
    if (DEFAULT_ENVIRONMENT == "local") {
        httpClient = &insecureClient;
    } else {
        secureClient.setCACert(rootCACertificate);
        httpClient = &secureClient;
    }
}

void FirmwareVersionTracker::retrieveLatestFirmwareFromServer(uint16_t newVersion) {
    if (isRetrievingFirmwareFromServer || WiFi.status() != WL_CONNECTED) {
        SerialQueueManager::getInstance().queueMessage("Cannot update: Either already updating or WiFi not connected");
        return;
    }

    if (firmwareVersion >= newVersion) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Pip is up to date. Current version is: %d, new version is: %d\n", firmwareVersion, newVersion);
        SerialQueueManager::getInstance().queueMessage(buffer);
        return;
    }

    pendingVersion = newVersion;  // Store for the callback to use
    isRetrievingFirmwareFromServer = true;

    // Get endpoint
    String url = getServerFirmwareEndpoint();
    SensorDataBuffer::getInstance().stopPollingAllSensors(); // Don't need sensors when updating

    // Perform the update
    t_httpUpdate_return result = httpUpdate.update(*httpClient, url);
    
    switch (result) {
        case HTTP_UPDATE_FAILED:
            isRetrievingFirmwareFromServer = false;
            break;
            
        case HTTP_UPDATE_NO_UPDATES:
            SerialQueueManager::getInstance().queueMessage("No updates needed");
            isRetrievingFirmwareFromServer = false;
            break;
            
        case HTTP_UPDATE_OK:
            // We'll never reach this because the device will restart after onEnd
            break;
    }
    
    isRetrievingFirmwareFromServer = false;
}

void FirmwareVersionTracker::updateProgressLeds(int progress, int total) {
    // Calculate percentage (0-100)
    int percentage = (progress * 100) / total;
    
    // Define segment boundaries (each LED represents 1/6 of the total)
    const int segmentSize = 100 / 6; // ~16.67%
    
    // Calculate which segment we're in (0-5)
    int segment = min(5, percentage / segmentSize);
    
    // Calculate progress within the current segment (0-100%)
    int segmentStart = segment * segmentSize;
    int segmentProgress = percentage - segmentStart;
    
    // Convert segment progress to brightness (0-255)
    uint8_t brightness = (segmentProgress * 255) / segmentSize;
    
    // Set all LEDs based on the current progress
    for (int i = 0; i < 6; i++) {
        uint8_t ledBrightness = 0;
        
        if (i < segment) {
            // Previous segments are fully lit
            ledBrightness = 255;
        } else if (i == segment) {
            // Current segment is partially lit
            ledBrightness = brightness;
        }
        // Future segments remain off (brightness = 0)
        
        // Set the appropriate LED
        switch(i) {
            case 0: // Bottom right
                rgbLed.set_back_right_led(0, ledBrightness, 0);
                break;
            case 1: // Middle right
                rgbLed.set_middle_right_led(0, ledBrightness, 0);
                break;
            case 2: // Top right
                rgbLed.set_top_right_led(0, ledBrightness, 0);
                break;
            case 3: // Top left
                rgbLed.set_top_left_led(0, ledBrightness, 0);
                break;
            case 4: // Middle left
                rgbLed.set_middle_left_led(0, ledBrightness, 0);
                break;
            case 5: // Bottom left
                rgbLed.set_back_left_led(0, ledBrightness, 0);
                break;
        }
    }
}
