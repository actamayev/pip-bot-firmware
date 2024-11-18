// #include "./include/show_chip_info.h"

// void printFlashInfo() {
//     // Flash chip info
//     Serial.println("\n=== Flash Chip Info ===");
//     Serial.printf("Flash Chip Size: %d bytes (%.2f MB)\n", 
//         ESP.getFlashChipSize(), 
//         ESP.getFlashChipSize() / 1024.0 / 1024.0);
//     Serial.printf("Flash Chip Speed: %d Hz\n", ESP.getFlashChipSpeed());
//     Serial.printf("Flash Chip Mode: %d\n", ESP.getFlashChipMode());

//     // Partition info
//     Serial.println("\n=== Partition Info ===");
//     esp_partition_iterator_t it;
//     const esp_partition_t* running = esp_ota_get_running_partition();

//     it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
//     Serial.println("App Partitions:");
//     while (it != NULL) {
//         const esp_partition_t* part = esp_partition_get(it);
//         Serial.printf("- %s: address 0x%x, size %d bytes%s\n", 
//             part->label, part->address, part->size,
//             (part == running) ? " (current)" : "");
//         it = esp_partition_next(it);
//     }
//     esp_partition_iterator_release(it);

//     it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
//     Serial.println("\nData Partitions:");
//     while (it != NULL) {
//         const esp_partition_t* part = esp_partition_get(it);
//         Serial.printf("- %s: address 0x%x, size %d bytes\n", 
//             part->label, part->address, part->size);
//         it = esp_partition_next(it);
//     }
//     esp_partition_iterator_release(it);

//     // Sketch info
//     Serial.println("\n=== Sketch Info ===");
//     Serial.printf("Sketch Size: %d bytes\n", ESP.getSketchSize());
//     Serial.printf("Free Sketch Space: %d bytes\n", ESP.getFreeSketchSpace());
    
//     // Memory info
//     Serial.println("\n=== Memory Info ===");
//     Serial.printf("Total Heap: %d bytes\n", ESP.getHeapSize());
//     Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
//     Serial.printf("PSRAM: %s\n", psramFound() ? "Yes" : "No");
//     if (psramFound()) {
//         Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
//         Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
//     }
// }
