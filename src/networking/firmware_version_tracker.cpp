#include "./firmware_version_tracker.h"

FirmwareVersionTracker::FirmwareVersionTracker() {
    firmwareVersion = preferences.getInt("fw_version", 0);
    pendingVersion = preferences.getInt("fw_pending", 0);
    Serial.printf("firmwareVersion %d\n", firmwareVersion);

    // Configure HTTPUpdate instance
    httpUpdate.onProgress([](int curr, int total) {
        Serial.printf("Update progress: %d%%\n", (curr * 100) / total);
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
    firmwareVersion = version;
    preferences.putInt("fw_version", version);
}

void FirmwareVersionTracker::setPendingVersion(int version) {
    pendingVersion = version;
    preferences.putInt("fw_pending", version);
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
    setPendingVersion(newVersion);
    isRetrievingFirmwareFromServer = true;

    // Get endpoint
    String url = getServerFirmwareEndpoint();
    Serial.printf("Update URL: %s\n", url.c_str());

    // Perform the update
    t_httpUpdate_return result = httpUpdate.update(*httpClient, url);
    
    switch (result) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP update failed: %s\n", httpUpdate.getLastErrorString().c_str());
            isRetrievingFirmwareFromServer = false;
            break;
            
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("No updates needed");
            isRetrievingFirmwareFromServer = false;
            break;
            
        case HTTP_UPDATE_OK:
            Serial.println("Update successful!");
            
            // If we have a pending version, apply it
            if (pendingVersion > 0) {
                setFirmwareVersion(pendingVersion);
                Serial.printf("Firmware version updated to: %d\n", pendingVersion);
                
                // Clear pending version
                setPendingVersion(0);
            }
            
            Serial.println("Rebooting...");
            // Device will automatically reboot
            break;
    }
    
    isRetrievingFirmwareFromServer = false;
}
