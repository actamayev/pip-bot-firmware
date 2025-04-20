#include "./firmware_version_tracker.h"

FirmwareVersionTracker::FirmwareVersionTracker() {
    preferences.begin("firmware", false);
    firmwareVersion = preferences.getInt("version", 0);
    preferences.end();
    
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

void FirmwareVersionTracker::retrieveLatestFirmwareFromServer() {
    if (isRetrievingFirmwareFromServer || WiFi.status() != WL_CONNECTED) {
        Serial.println("Cannot update: Either already updating or WiFi not connected");
        return;
    }
    
    isRetrievingFirmwareFromServer = true;
    Serial.println("Checking for firmware updates...");
    
    // Add current version as header for the server to check
    httpUpdate.addHeader("x-esp32-version", String(firmwareVersion));
    
    // Optional: Set longer timeout for large firmware files
    httpUpdate.setTimeout(120000); // 2 minutes
    
    // Perform the update
    t_httpUpdate_return result = httpUpdate.update(client, "https://your-server.com/firmware");
    
    switch (result) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP update failed: %s\n", httpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("No updates needed");
            break;
        case HTTP_UPDATE_OK:
            Serial.println("Update successful! Rebooting...");
            // Device will automatically reboot
            break;
    }
    
    isRetrievingFirmwareFromServer = false;
}