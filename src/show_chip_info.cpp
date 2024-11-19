#include "./include/show_chip_info.h"

void printFlashInfo() {
    delay(100);  // Give some time for initialization
    
    // Flash chip info
    Serial.println("\n=== Flash Chip Info ===");
    Serial.printf("Flash Chip Size: %d bytes (%.2f MB)\n", 
        ESP.getFlashChipSize(), 
        ESP.getFlashChipSize() / 1024.0 / 1024.0);
    Serial.printf("Flash Chip Speed: %d Hz\n", ESP.getFlashChipSpeed());
    
    // Memory info
    Serial.println("\n=== Memory Info ===");
    Serial.printf("Total Heap: %d bytes\n", ESP.getHeapSize());
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    
    // PSRAM Check
    Serial.println("\n=== PSRAM Info ===");
    if (psramFound()) {
        size_t psram_size = ESP.getPsramSize();
        if (psram_size > 0) {
            Serial.println("PSRAM is available!");
            Serial.printf("Total PSRAM: %d bytes (%.2f MB)\n", 
                psram_size, 
                psram_size / 1024.0 / 1024.0);
            Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
            
            // Test PSRAM
            Serial.println("Testing PSRAM...");
            void* ptr = ps_malloc(1024);  // Try to allocate 1KB
            if (ptr != NULL) {
                Serial.println("Successfully allocated memory in PSRAM");
                free(ptr);
            } else {
                Serial.println("Failed to allocate memory in PSRAM");
            }
        } else {
            Serial.println("PSRAM found but size is 0!");
        }
    } else {
        Serial.println("No PSRAM detected!");
        Serial.println("Check your board_build.psram settings in platformio.ini");
    }

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
}
