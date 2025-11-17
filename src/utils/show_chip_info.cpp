#include "show_chip_info.h"

void print_flash_info() {
    vTaskDelay(pdMS_TO_TICKS(100)); // Give some time for initialization

    // Flash chip info
    SerialQueueManager::get_instance().queue_message("\n=== Flash Chip Info ===");
    // SerialQueueManager::get_instance().queue_message("Flash Chip Size: %d bytes (%.2f MB)\n",
    // ESP.getFlashChipSize(),
    // ESP.getFlashChipSize() / 1024.0 / 1024.0);
    // SerialQueueManager::get_instance().queue_message("Flash Chip Speed: %d Hz\n", ESP.getFlashChipSpeed());

    // Memory info
    // SerialQueueManager::get_instance().queue_message("\n=== Memory Info ===");
    // SerialQueueManager::get_instance().queue_message("Total Heap: %d bytes\n", ESP.getHeapSize());
    // SerialQueueManager::get_instance().queue_message("Free Heap: %d bytes\n", ESP.getFreeHeap());

    // Partition info
    SerialQueueManager::get_instance().queue_message("\n=== Partition Info ===");
    esp_partition_iterator_t it = nullptr;
    const esp_partition_t* running = esp_ota_get_running_partition();

    it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    SerialQueueManager::get_instance().queue_message("App Partitions:");
    while (it != nullptr) {
        const esp_partition_t* part = esp_partition_get(it);
        // SerialQueueManager::get_instance().queue_message("- %s: address 0x%x, size %d bytes%s\n",
        //     part->label, part->address, part->size,
        //     (part == running) ? " (current)" : "");
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
}
