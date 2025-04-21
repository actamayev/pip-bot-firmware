#include "./firmware_version_tracker.h"

FirmwareVersionTracker::FirmwareVersionTracker() {
    preferences.begin("firmware", false);
    
    firmwareVersion = preferences.getInt("fw_version", 0);
    pendingVersion = preferences.getInt("fw_pending", 0);
    
    Serial.printf("Constructor - firmwareVersion: %d, pendingVersion: %d\n", firmwareVersion, pendingVersion);
    
    // Check if we've just completed an update
    if (pendingVersion > 0) {
        Serial.printf("Applying pending update from version %d to version %d\n", firmwareVersion, pendingVersion);
        
        // Update the firmware version
        setFirmwareVersion(pendingVersion);
        Serial.printf("Firmware version updated to: %d\n", pendingVersion);
        
        // Clear pending version
        setPendingVersion(0);
        Serial.println("Pending version cleared");
    }
    
    // Close preferences when done
    preferences.end();

    // Configure HTTPUpdate instance
    httpUpdate.onProgress([this](int curr, int total) {
        Serial.printf("Update progress: %d%%\n", (curr * 100) / total);
        this->updateProgressLeds(curr, total);
    });

    // Setup clients based on environment
    if (DEFAULT_ENVIRONMENT == "local") {
        httpClient = &insecureClient;
    } else {
        secureClient.setCACert(rootCACertificate);
        httpClient = &secureClient;
    }
}

void FirmwareVersionTracker::setFirmwareVersion(int version) {
    preferences.begin("firmware", false);
    firmwareVersion = version;
    preferences.putInt("fw_version", version);
    preferences.end();
}

void FirmwareVersionTracker::setPendingVersion(int version) {
    preferences.begin("firmware", false);
    pendingVersion = version;
    preferences.putInt("fw_pending", version);
    preferences.end();
}

void FirmwareVersionTracker::retrieveLatestFirmwareFromServer(uint16_t newVersion) {
    if (isRetrievingFirmwareFromServer || WiFi.status() != WL_CONNECTED) {
        Serial.println("Cannot update: Either already updating or WiFi not connected");
        return;
    }

    if (firmwareVersion >= newVersion) {
        Serial.printf("Pip is up to date. Current version is: %d, new version is: %d\n", firmwareVersion, newVersion);
        return;
    }

    Serial.printf("Starting update to version %d...\n", newVersion);
    Serial.printf("Setting pending version to: %d\n", newVersion);
    setPendingVersion(newVersion); // The pending version is retrieved after the ESP restarts after the update is complete
    isRetrievingFirmwareFromServer = true;

    // Get endpoint
    String url = getServerFirmwareEndpoint();
    Serial.printf("Update URL: %s\n", url.c_str());

    // Perform the update
    t_httpUpdate_return result = httpUpdate.update(*httpClient, url);
    
    switch (result) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP update failed: %s\n", httpUpdate.getLastErrorString().c_str());
            Serial.println("Clearing pending version due to update failure");
            setPendingVersion(0);
            isRetrievingFirmwareFromServer = false;
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("No updates needed");
            Serial.println("Clearing pending version as no update was performed");
            setPendingVersion(0);
            isRetrievingFirmwareFromServer = false;
            break;

        case HTTP_UPDATE_OK:
            Serial.println("Update successful!");
            // We won't reach this code because the ESP automatically restarts
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
