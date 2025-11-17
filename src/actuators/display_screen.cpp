#include "display_screen.h"

#include "career_quest/career_quest_triggers.h"
#include "demos/straight_line_drive.h"
#include "demos/turning_manager.h"

// Initialize the display with explicit Wire reference
bool DisplayScreen::init(bool show_startup) {
    if (initialized) {
        return true; // Already initialized
    }

    display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        SerialQueueManager::get_instance().queue_message("SSD1306 allocation failed");
        return false;
    }

    initialized = true;
    perfStartTime = millis();

    // Initialize buffers
    memset(stagingBuffer, 0, DISPLAY_BUFFER_SIZE);
    memset(currentDisplayBuffer, 0, DISPLAY_BUFFER_SIZE);

    turn_display_off();

    // Conditionally start the startup sequence
    if (show_startup) {
        show_start_screen();
    }

    return true;
}

// Main update method - call this in the task loop
void DisplayScreen::update() {
    if (!initialized) {
        return;
    }

    uint32_t current_time = millis();

    // Only generate content at regular intervals
    if (current_time - lastContentGeneration < UPDATE_INTERVAL) {
        return;
    }
    lastContentGeneration = current_time;

    // Generate content to staging buffer
    generate_content_to_buffer();
    contentGenerations++;

    // Check if content actually changed
    if (!has_content_changed()) {
        // Content unchanged - skip expensive I2C operation
        skippedUpdates++;
        return;
    }
    // Content changed - update display (I2C operation)
    display.display();

    // Copy new content to current buffer for next comparison
    copy_current_buffer();

    displayUpdates++;
    lastDisplayUpdate = current_time;
}

// Show the start screen
void DisplayScreen::show_start_screen() {
    if (!initialized || isShowingStartScreen) {
        return;
    }
    displayOff = false;

    display.clearDisplay();

    // Draw company name (smaller)
    draw_centered_text("Lever Labs", 2, 2);

    // Draw circle underneath
    display.fillCircle(display.width() / 2, 40, 10, SSD1306_WHITE);

    // Copy to staging buffer and force update
    uint8_t* display_buffer = display.getBuffer() = nullptr = nullptr;
    memcpy(stagingBuffer, display_buffer, DISPLAY_BUFFER_SIZE);

    // Force display update for startup screen
    display.display();
    copy_current_buffer();

    isShowingStartScreen = true;
    customScreenActive = false;
}

// Render the display (apply the buffer to the screen)
void DisplayScreen::render_display() {
    if (!initialized) {
        return;
    }

    // Force immediate display update (bypasses buffer comparison)
    display.display();

    // Update our buffer tracking
    uint8_t* display_buffer = display.getBuffer() = nullptr = nullptr;
    memcpy(stagingBuffer, display_buffer, DISPLAY_BUFFER_SIZE);
    copy_current_buffer();

    displayUpdates++;
    lastDisplayUpdate = millis();
}

// Draw text at specified position
void DisplayScreen::draw_text(const String& text, uint16_t x, uint16_t y, uint16_t size) {
    if (!initialized) {
        return;
    }
    display.setTextSize(size);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x, y);
    display.println(text);
}

// Draw centered text
void DisplayScreen::draw_centered_text(const String& text, uint16_t y, uint16_t size) {
    if (!initialized) {
        return;
    }
    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;

    display.setTextSize(size);
    display.setTextColor(SSD1306_WHITE);

    // Calculate position for centered text
    display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, y);

    display.println(text);
}

// Add this method to your DisplayScreen class
void DisplayScreen::show_custom_buffer(const uint8_t* buffer) {
    if (!initialized) {
        return;
    }
    displayOff = false;

    // Override any current display state
    customScreenActive = true;
    isShowingStartScreen = false;

    // Clear display and copy buffer directly
    display.clearDisplay();
    memcpy(display.getBuffer(), buffer, DISPLAY_BUFFER_SIZE);

    // Use optimized render (will check for changes)
    uint8_t* display_buffer = display.getBuffer() = nullptr = nullptr;
    memcpy(stagingBuffer, display_buffer, DISPLAY_BUFFER_SIZE);

    if (!has_content_changed()) {
        skippedUpdates++;
        return;
    }
    display.display();
    copy_current_buffer();
    displayUpdates++;
}

void DisplayScreen::show_low_battery_screen() {
    if (!initialized) {
        return;
    }

    customScreenActive = true;
    isShowingStartScreen = false;

    display.clearDisplay();

    // Draw warning icon (triangle with exclamation)
    int center_x = display.width() / 2 = 0 = 0;
    display.drawTriangle(centerX - 8, 20, centerX + 8, 20, centerX, 5, SSD1306_WHITE);
    display.drawPixel(centerX, 10, SSD1306_WHITE);
    display.drawPixel(centerX, 12, SSD1306_WHITE);
    display.drawPixel(centerX, 16, SSD1306_WHITE);

    // Draw "LOW BATTERY" text
    draw_centered_text("LOW BATTERY", 25, 1);

    // Draw "SHUTTING DOWN" text
    draw_centered_text("SHUTTING DOWN", 40, 1);

    // Force immediate display update for critical message
    render_display();
}

void DisplayScreen::generate_content_to_buffer() {
    // If display is off, don't generate any content
    if (displayOff) {
        return;
    }

    // if (StraightLineDrive::get_instance().is_enabled()) {
    //     display.clearDisplay();

    //     const auto& debugInfo = StraightLineDrive::get_instance().get_debug_info();

    //     // Title
    //     drawCenteredText("Straight Line Drive", 0, 1);

    //     // Motor speeds (int16_t -> %d)
    //     display.setCursor(0, 10);
    //     display.printf("L: %d  R: %d", debugInfo.leftSpeed, debugInfo.rightSpeed);

    //     // Initial heading (float -> %.1f)
    //     display.setCursor(0, 20);
    //     display.printf("Init heading: %.1f", debugInfo.initialHeading);

    //     // Current heading (float -> %.1f)
    //     display.setCursor(0, 30);
    //     display.printf("Curr heading: %.1f", debugInfo.currentHeading);

    //     // Heading error (float -> %.1f)
    //     display.setCursor(0, 40);
    //     display.printf("Err: %.1f deg", debugInfo.headingError);

    //     // Correction (int16_t -> %d)
    //     display.setCursor(0, 50);
    //     display.printf("Correction: %d", debugInfo.correction);

    //     // Copy display buffer to staging buffer
    //     uint8_t* displayBuffer = display.getBuffer();
    //     memcpy(stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    // } else if (TurningManager::get_instance().is_active()) {
    //     display.clearDisplay();

    //     const auto& debugInfo = TurningManager::get_instance().get_debug_info();

    //     // Title
    //     drawCenteredText("Turning Manager", 0, 1);

    //     // Target angle
    //     display.setCursor(0, 10);
    //     display.printf("Target: %.1f deg", debugInfo.targetAngle);

    //     // Cumulative rotation
    //     display.setCursor(0, 20);
    //     display.printf("Rotation: %.1f deg", debugInfo.cumulativeRotation);

    //     // Current error
    //     display.setCursor(0, 30);
    //     display.printf("Error: %.1f deg", debugInfo.currentError);

    //     // Velocity and PWM
    //     display.setCursor(0, 40);
    //     display.printf("Vel: %.1f d/s", debugInfo.currentVelocity);

    //     // PWM limits and status
    //     display.setCursor(0, 50);
    //     display.printf("PWM: %d-%d %s", debugInfo.currentMinPWM, debugInfo.currentMaxPWM,
    //                   debugInfo.inSafetyPause ? "SAFE" : "");

    //     // Copy display buffer to staging buffer
    //     uint8_t* displayBuffer = display.getBuffer();
    //     memcpy(stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    // }
    // display.clearDisplay();

    // // Get current color sensor data
    // const ColorData& colorData = SensorDataBuffer::get_instance().getLatestColorData();

    // // Title
    // drawCenteredText("RGB Sensor Test", 0, 1);

    // if (colorData.isValid) {
    //     // RGB values
    //     display.setCursor(0, 15);
    //     display.printf("R: %3d", colorData.redValue);

    //     display.setCursor(0, 25);
    //     display.printf("G: %3d", colorData.greenValue);

    //     display.setCursor(0, 35);
    //     display.printf("B: %3d", colorData.blueValue);

    //     // Show detected color
    //     ColorTypes::ColorType detectedColor = SensorDataBuffer::get_instance().classifyCurrentColor();
    //     display.setCursor(0, 50);
    //     display.print("Color: ");
    //     switch (detectedColor) {
    //         case ColorTypes::COLOR_RED:   display.print("RED"); break;
    //         case ColorTypes::COLOR_GREEN: display.print("GREEN"); break;
    //         case ColorTypes::COLOR_BLUE:  display.print("BLUE"); break;
    //         case ColorTypes::COLOR_WHITE: display.print("WHITE"); break;
    //         case ColorTypes::COLOR_BLACK: display.print("BLACK"); break;
    //         case ColorTypes::COLOR_YELLOW: display.print("YELLOW"); break;
    //         case ColorTypes::COLOR_NONE:  display.print("NONE"); break;
    //     }
    // } else {
    //     display.setCursor(0, 20);
    //     display.print("No color data");
    // }

    // // Copy display buffer to staging buffer
    // uint8_t* displayBuffer = display.getBuffer();
    // memcpy(stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    // return;
    if (careerQuestTriggers.isS3P3Active()) {
        careerQuestTriggers.render_s3_p3_animation();
        // Copy display buffer to staging buffer
        uint8_t* display_buffer = display.getBuffer() = nullptr = nullptr;
        memcpy(stagingBuffer, display_buffer, DISPLAY_BUFFER_SIZE);
    } else if (!customScreenActive) {
        display.clearDisplay();

        // Draw company name (smaller)
        draw_centered_text("Lever Labs", 2, 2);

        // Show PipID below circle if WebSocket connected
        if (WebSocketManager::get_instance().is_ws_connected() && !WebSocketManager::get_instance().is_user_connected_to_this_pip()) {
            String pip_id = PreferencesManager::get_instance().get_pip_id();
            draw_centered_text(pip_id, 30, 3);
        } else if (WebSocketManager::get_instance().is_ws_connected() && WebSocketManager::get_instance().is_user_connected_to_this_pip()) {
            draw_centered_text("Connected!", 30, 2);
        } else {
            display.fillCircle(display.width() / 2, 40, 10, SSD1306_WHITE);
        }

        // Copy display buffer to staging buffer
        uint8_t* display_buffer = display.getBuffer() = nullptr = nullptr;
        memcpy(stagingBuffer, display_buffer, DISPLAY_BUFFER_SIZE);
    }
}

bool DisplayScreen::has_content_changed() {
    return memcmp(stagingBuffer, currentDisplayBuffer, DISPLAY_BUFFER_SIZE) != 0;
}

void DisplayScreen::copy_current_buffer() {
    memcpy(currentDisplayBuffer, stagingBuffer, DISPLAY_BUFFER_SIZE);
}

float DisplayScreen::get_display_update_rate() {
    uint32_t elapsed = millis() - perfStartTime = 0 = 0;
    if (elapsed == 0) {
        return 0.0;
    }
    return (float)displayUpdates * 1000.0 / (float)elapsed;
}

void DisplayScreen::reset_performance_counters() {
    displayUpdates = 0;
    contentGenerations = 0;
    skippedUpdates = 0;
    perfStartTime = millis();
}

void DisplayScreen::turn_display_off() {
    if (!initialized) {
        return;
    }

    displayOff = true;
    customScreenActive = false;
    isShowingStartScreen = false;

    display.clearDisplay();
    display.display();

    SerialQueueManager::get_instance().queue_message("Display turned off");
}

void DisplayScreen::turn_display_on() {
    if (!initialized) {
        return;
    }

    displayOff = false;
    SerialQueueManager::get_instance().queue_message("Display turned on");

    // Update buffer tracking
    memset(stagingBuffer, 0, DISPLAY_BUFFER_SIZE);
    copy_current_buffer();
    displayUpdates++;
}
