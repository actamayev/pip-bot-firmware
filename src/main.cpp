#include "./include/config.h"
#include "./include/wifi_manager.h"
#include "./include/sensor_setup.h"
#include "./include/esp32_api_client.h"
#include "./include/webserver_manager.h"
#include "./include/user_code.h"

#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_flash.h"

void printFlashInfo() {
    // Flash chip info
    Serial.println("\n=== Flash Chip Info ===");
    Serial.printf("Flash Chip Size: %d bytes (%.2f MB)\n", 
        ESP.getFlashChipSize(), 
        ESP.getFlashChipSize() / 1024.0 / 1024.0);
    Serial.printf("Flash Chip Speed: %d Hz\n", ESP.getFlashChipSpeed());
    Serial.printf("Flash Chip Mode: %d\n", ESP.getFlashChipMode());

    // Partition info
    Serial.println("\n=== Partition Info ===");
    esp_partition_iterator_t it;
    const esp_partition_t* running = esp_ota_get_running_partition();

    it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    Serial.println("App Partitions:");
    while (it != NULL) {
        const esp_partition_t* part = esp_partition_get(it);
        Serial.printf("- %s: address 0x%x, size %d bytes%s\n", 
            part->label, part->address, part->size,
            (part == running) ? " (current)" : "");
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);

    it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
    Serial.println("\nData Partitions:");
    while (it != NULL) {
        const esp_partition_t* part = esp_partition_get(it);
        Serial.printf("- %s: address 0x%x, size %d bytes\n", 
            part->label, part->address, part->size);
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);

    // Sketch info
    Serial.println("\n=== Sketch Info ===");
    Serial.printf("Sketch Size: %d bytes\n", ESP.getSketchSize());
    Serial.printf("Free Sketch Space: %d bytes\n", ESP.getFreeSketchSpace());
    
    // Memory info
    Serial.println("\n=== Memory Info ===");
    Serial.printf("Total Heap: %d bytes\n", ESP.getHeapSize());
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("PSRAM: %s\n", psramFound() ? "Yes" : "No");
    if (psramFound()) {
        Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    }
}

// This task will run user code on Core 0
void UserCodeTask(void * parameter) {
    for(;;) {
        user_code();  // Your LED blinking code
        delay(1);     // Small delay to prevent watchdog reset
    }
}

// Main setup runs on Core 1
void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.printf("setup() running on Core %d\n", xPortGetCoreID());

    printFlashInfo();

    // Setup WiFi, sensors, etc.
    apiClient = new ESP32ApiClient();
    sensorSetup.sensor_setup();
    wifiManager.initializeWiFi();
    wifiManager.connectToStoredWiFi();

    // Create task for user code on Core 0
    xTaskCreatePinnedToCore(
        UserCodeTask,  // Function to run
        "UserCode",    // Task name
        10000,         // Stack size
        NULL,          // Task parameters
        1,             // Priority
        NULL,          // Task handle
        0              // Run on Core 0
    );
}

// Main loop runs on Core 1
void loop() {
    // Network-related tasks on Core 1
    webServerManager.handleClientRequests();
    
    if (WiFi.status() == WL_CONNECTED) {
        apiClient->pollWebSocket();
    }
}
