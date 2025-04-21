#include "./firmware_version_tracker.h"

FirmwareVersionTracker::FirmwareVersionTracker() {
    preferences.begin("firmware", false);
    firmwareVersion = preferences.getInt("version", 0);
    preferences.end();
    Serial.printf("firmwareVersion %d\n", firmwareVersion);
    // Configure HTTPUpdate instance
    httpUpdate.onProgress([](int curr, int total) {
        Serial.printf("Update progress: %d%%\n", (curr * 100) / total);
    });
}

void FirmwareVersionTracker::setFirmwareVersion(int version) {
    firmwareVersion = version;
    preferences.begin("firmware", false);
    preferences.putInt("version", version);
    preferences.end();
}

void FirmwareVersionTracker::setPendingVersion(int version) {
    pendingVersion = version;
    preferences.begin("firmware", false);
    preferences.putInt("pending", version);
    preferences.end();
}

void FirmwareVersionTracker::retrieveLatestFirmwareFromServer() {
    if (isRetrievingFirmwareFromServer || WiFi.status() != WL_CONNECTED) {
        Serial.println("Cannot update: Either already updating or WiFi not connected");
        return;
    }

    isRetrievingFirmwareFromServer = true;
    
    // Perform the update
    t_httpUpdate_return result = httpUpdate.update(client, getServerFirmwareEndpoint());
    
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
