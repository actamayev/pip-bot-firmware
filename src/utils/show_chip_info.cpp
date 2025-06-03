#include "show_chip_info.h"

void printFlashInfo() {
    vTaskDelay(pdMS_TO_TICKS(100));  // Give some time for initialization

    // Flash chip info
    SerialQueueManager::getInstance().queueMessage("\n=== Flash Chip Info ===");
    // SerialQueueManager::getInstance().queueMessage("Flash Chip Size: %d bytes (%.2f MB)\n", 
    // ESP.getFlashChipSize(), 
    // ESP.getFlashChipSize() / 1024.0 / 1024.0);
    // SerialQueueManager::getInstance().queueMessage("Flash Chip Speed: %d Hz\n", ESP.getFlashChipSpeed());

    // Memory info
    // SerialQueueManager::getInstance().queueMessage("\n=== Memory Info ===");
    // SerialQueueManager::getInstance().queueMessage("Total Heap: %d bytes\n", ESP.getHeapSize());
    // SerialQueueManager::getInstance().queueMessage("Free Heap: %d bytes\n", ESP.getFreeHeap());

    // Partition info
    SerialQueueManager::getInstance().queueMessage("\n=== Partition Info ===");
    esp_partition_iterator_t it;
    const esp_partition_t* running = esp_ota_get_running_partition();

    it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    SerialQueueManager::getInstance().queueMessage("App Partitions:");
    while (it != NULL) {
        const esp_partition_t* part = esp_partition_get(it);
        // SerialQueueManager::getInstance().queueMessage("- %s: address 0x%x, size %d bytes%s\n", 
        //     part->label, part->address, part->size,
        //     (part == running) ? " (current)" : "");
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
}
