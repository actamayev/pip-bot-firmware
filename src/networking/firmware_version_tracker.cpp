#include "firmware_version_tracker.h"

FirmwareVersionTracker::FirmwareVersionTracker() {
    firmware_version = PreferencesManager::get_instance().get_firmware_version();
    // Format it into a message
    char message[64];
    snprintf(message, sizeof(message), "Firmware Version: %d", firmware_version);

    // Queue the message via SerialQueueManager
    SerialQueueManager::get_instance().queue_message(message, SerialPriority::NORMAL);

    // Configure HTTPUpdate instance
    http_update.onProgress([this](int curr, int total) { this->update_progress_leds(curr, total); });

    // Set onEnd callback to update firmware version before reboot
    http_update.onEnd([this]() { PreferencesManager::get_instance().set_firmware_version(this->pending_version); });

    // Setup clients based on environment
    if (DEFAULT_ENVIRONMENT == "local") {
        http_client = &insecure_client;
    } else {
        secure_client.setCACert(ROOT_CA_CERTIFICATE);
        http_client = &secure_client;
    }
}

void FirmwareVersionTracker::retrieve_latest_firmware_from_server(uint16_t new_version) {
    if (is_retrieving_firmware_from_server || WiFi.status() != WL_CONNECTED) {
        SerialQueueManager::get_instance().queue_message("Cannot update: Either already updating or WiFi not connected");
        return;
    }

    // Format it into a message
    char message[64];
    snprintf(message, sizeof(message), "New Firmware Version: %d", new_version);
    SerialQueueManager::get_instance().queue_message(message, SerialPriority::NORMAL);

    if (firmware_version >= new_version) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Pip is up to date. Current version is: %d, new version is: %d\n", firmware_version, new_version);
        SerialQueueManager::get_instance().queue_message(buffer);
        return;
    }

    pending_version = new_version; // Store for the callback to use
    is_retrieving_firmware_from_server = true;

    // Get endpoint
    String url = getServerFirmwareEndpoint();
    CareerQuestTriggers::get_instance().stop_all_career_quest_triggers(true); // Stop all sensors, movement when updating

    // Perform the update
    t_httpUpdate_return result = http_update.update(*http_client, url);

    switch (result) {
        case HTTP_UPDATE_FAILED:
            is_retrieving_firmware_from_server = false;
            break;

        case HTTP_UPDATE_NO_UPDATES:
            SerialQueueManager::get_instance().queue_message("No updates needed");
            is_retrieving_firmware_from_server = false;
            break;

        case HTTP_UPDATE_OK:
            // We'll never reach this because the device will restart after onEnd
            break;
    }

    is_retrieving_firmware_from_server = false;
}

void FirmwareVersionTracker::update_progress_leds(int progress, int total) {
    // Stop any active LED animations to prevent interference
    LedAnimations::get_instance().stop_animation();

    // Calculate percentage (0-100)
    int percentage = (progress * 100) / total;

    // Define segment boundaries (each LED represents 1/6 of the total)
    const int segment_size = 100 / 6; // ~16.67%

    // Calculate which segment we're in (0-5)
    int segment = min(5, percentage / segment_size);

    // Calculate progress within the current segment (0-100%)
    int segment_start = segment * segment_size;
    int segment_progress = percentage - segment_start;

    // Convert segment progress to brightness (0-255)
    uint8_t brightness = (segment_progress * 255) / segment_size;

    // Set all LEDs based on the current progress
    for (int i = 0; i < 6; i++) {
        uint8_t led_brightness = 0;

        if (i < segment) {
            // Previous segments are fully lit
            led_brightness = 255;
        } else if (i == segment) {
            // Current segment is partially lit
            led_brightness = brightness;
        }
        // Future segments remain off (brightness = 0)

        // Set the appropriate LED
        switch (i) {
            case 0: // Bottom right
                RgbLed::get_instance().set_back_right_led(0, led_brightness, 0);
                break;
            case 1: // Middle right
                RgbLed::get_instance().set_middle_right_led(0, led_brightness, 0);
                break;
            case 2: // Top right
                RgbLed::get_instance().set_top_right_led(0, led_brightness, 0);
                break;
            case 3: // Top left
                RgbLed::get_instance().set_top_left_led(0, led_brightness, 0);
                break;
            case 4: // Middle left
                RgbLed::get_instance().set_middle_left_led(0, led_brightness, 0);
                break;
            case 5: // Bottom left
                RgbLed::get_instance().set_back_left_led(0, led_brightness, 0);
                break;
        }
    }
}
