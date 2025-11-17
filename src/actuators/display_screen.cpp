#include "display_screen.h"

#include "career_quest/career_quest_triggers.h"
#include "demos/straight_line_drive.h"
#include "demos/turning_manager.h"

// Initialize the display with explicit Wire reference
bool DisplayScreen::init(bool show_startup) {
    DisplayScreen& instance = DisplayScreen::get_instance();
    if (instance._initialized) {
        return true; // Already initialized
    }

    instance._display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if (!instance._display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        SerialQueueManager::get_instance().queue_message("SSD1306 allocation failed");
        return false;
    }

    instance._initialized = true;
    instance._perfStartTime = millis();

    // Initialize buffers
    memset(instance._stagingBuffer, 0, DISPLAY_BUFFER_SIZE);
    memset(instance._currentDisplayBuffer, 0, DISPLAY_BUFFER_SIZE);

    turn_display_off();

    // Conditionally start the startup sequence
    if (show_startup) {
        show_start_screen();
    }

    return true;
}

// Main update method - call this in the task loop
void DisplayScreen::update() {
    DisplayScreen& instance = DisplayScreen::get_instance();
    if (!instance._initialized) {
        return;
    }

    uint32_t current_time = millis();

    // Only generate content at regular intervals
    if (current_time - instance._lastContentGeneration < UPDATE_INTERVAL) {
        return;
    }
    instance._lastContentGeneration = current_time;

    // Generate content to staging buffer
    generate_content_to_buffer();
    instance._contentGenerations++;

    // Check if content actually changed
    if (!has_content_changed()) {
        // Content unchanged - skip expensive I2C operation
        instance._skippedUpdates++;
        return;
    }
    // Content changed - update display (I2C operation)
    instance._display.display();

    // Copy new content to current buffer for next comparison
    copy_current_buffer();

    instance._displayUpdates++;
    instance._lastDisplayUpdate = current_time;
}

// Show the start screen
void DisplayScreen::show_start_screen() {
    DisplayScreen& instance = DisplayScreen::get_instance();
    if (!instance._initialized || instance._isShowingStartScreen) {
        return;
    }
    instance._displayOff = false;

    instance._display.clearDisplay();

    // Draw company name (smaller)
    draw_centered_text("Lever Labs", 2, 2);

    // Draw circle underneath
    instance._display.fillCircle(instance._display.width() / 2, 40, 10, SSD1306_WHITE);

    // Copy to staging buffer and force update
    uint8_t* display_buffer = instance._display.getBuffer();
    memcpy(instance._stagingBuffer, display_buffer, DISPLAY_BUFFER_SIZE);

    // Force display update for startup screen
    instance._display.display();
    copy_current_buffer();

    instance._isShowingStartScreen = true;
    instance._customScreenActive = false;
}

// Render the display (apply the buffer to the screen)
void DisplayScreen::render_display() {
    DisplayScreen& instance = DisplayScreen::get_instance();
    if (!instance._initialized) {
        return;
    }

    // Force immediate display update (bypasses buffer comparison)
    instance._display.display();

    // Update our buffer tracking
    uint8_t* display_buffer = instance._display.getBuffer();
    memcpy(instance._stagingBuffer, display_buffer, DISPLAY_BUFFER_SIZE);
    copy_current_buffer();

    instance._displayUpdates++;
    instance._lastDisplayUpdate = millis();
}

// Draw text at specified position
void DisplayScreen::draw_text(const String& text, uint16_t x, uint16_t y, uint16_t size) {
    DisplayScreen& instance = DisplayScreen::get_instance();
    if (!instance._initialized) {
        return;
    }
    instance._display.setTextSize(size);
    instance._display.setTextColor(SSD1306_WHITE);
    instance._display.setCursor(x, y);
    instance._display.println(text);
}

// Draw centered text
void DisplayScreen::draw_centered_text(const String& text, uint16_t y, uint16_t size) {
    DisplayScreen& instance = DisplayScreen::get_instance();
    if (!instance._initialized) {
        return;
    }
    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;

    instance._display.setTextSize(size);
    instance._display.setTextColor(SSD1306_WHITE);

    // Calculate position for centered text
    instance._display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    instance._display.setCursor((SCREEN_WIDTH - w) / 2, y);

    instance._display.println(text);
}

// Add this method to your DisplayScreen class
void DisplayScreen::show_custom_buffer(const uint8_t* buffer) {
    DisplayScreen& instance = DisplayScreen::get_instance();
    if (!instance._initialized) {
        return;
    }
    instance._displayOff = false;

    // Override any current display state
    instance._customScreenActive = true;
    instance._isShowingStartScreen = false;

    // Clear display and copy buffer directly
    instance._display.clearDisplay();
    memcpy(instance._display.getBuffer(), buffer, DISPLAY_BUFFER_SIZE);

    // Use optimized render (will check for changes)
    uint8_t* display_buffer = instance._display.getBuffer();
    memcpy(instance._stagingBuffer, display_buffer, DISPLAY_BUFFER_SIZE);

    if (!has_content_changed()) {
        instance._skippedUpdates++;
        return;
    }
    instance._display.display();
    copy_current_buffer();
    instance._displayUpdates++;
}

void DisplayScreen::show_low_battery_screen() {
    DisplayScreen& instance = DisplayScreen::get_instance();
    if (!instance._initialized) {
        return;
    }

    instance._customScreenActive = true;
    instance._isShowingStartScreen = false;

    instance._display.clearDisplay();

    // Draw warning icon (triangle with exclamation)
    int center_x = instance._display.width() / 2;
    instance._display.drawTriangle(center_x - 8, 20, center_x + 8, 20, center_x, 5, SSD1306_WHITE);
    instance._display.drawPixel(center_x, 10, SSD1306_WHITE);
    instance._display.drawPixel(center_x, 12, SSD1306_WHITE);
    instance._display.drawPixel(center_x, 16, SSD1306_WHITE);

    // Draw "LOW BATTERY" text
    draw_centered_text("LOW BATTERY", 25, 1);

    // Draw "SHUTTING DOWN" text
    draw_centered_text("SHUTTING DOWN", 40, 1);

    // Force immediate display update for critical message
    render_display();
}

void DisplayScreen::generate_content_to_buffer() {
    DisplayScreen& instance = DisplayScreen::get_instance();
    // If display is off, don't generate any content
    if (instance._displayOff) {
        return;
    }

    // if (StraightLineDrive::get_instance().is_enabled()) {
    //     _display.clearDisplay();

    //     const auto& debugInfo = StraightLineDrive::get_instance().get_debug_info();

    //     // Title
    //     drawCenteredText("Straight Line Drive", 0, 1);

    //     // Motor speeds (int16_t -> %d)
    //     _display.setCursor(0, 10);
    //     _display.printf("L: %d  R: %d", debugInfo.leftSpeed, debugInfo.rightSpeed);

    //     // Initial heading (float -> %.1f)
    //     _display.setCursor(0, 20);
    //     _display.printf("Init heading: %.1f", debugInfo.initialHeading);

    //     // Current heading (float -> %.1f)
    //     _display.setCursor(0, 30);
    //     _display.printf("Curr heading: %.1f", debugInfo.currentHeading);

    //     // Heading error (float -> %.1f)
    //     _display.setCursor(0, 40);
    //     _display.printf("Err: %.1f deg", debugInfo.headingError);

    //     // Correction (int16_t -> %d)
    //     _display.setCursor(0, 50);
    //     _display.printf("Correction: %d", debugInfo.correction);

    //     // Copy display buffer to staging buffer
    //     uint8_t* displayBuffer = _display.getBuffer();
    //     memcpy(_stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    // } else if (TurningManager::get_instance().is_active()) {
    //     _display.clearDisplay();

    //     const auto& debugInfo = TurningManager::get_instance().get_debug_info();

    //     // Title
    //     drawCenteredText("Turning Manager", 0, 1);

    //     // Target angle
    //     _display.setCursor(0, 10);
    //     _display.printf("Target: %.1f deg", debugInfo.targetAngle);

    //     // Cumulative rotation
    //     _display.setCursor(0, 20);
    //     _display.printf("Rotation: %.1f deg", debugInfo.cumulativeRotation);

    //     // Current error
    //     _display.setCursor(0, 30);
    //     _display.printf("Error: %.1f deg", debugInfo.currentError);

    //     // Velocity and PWM
    //     _display.setCursor(0, 40);
    //     _display.printf("Vel: %.1f d/s", debugInfo.currentVelocity);

    //     // PWM limits and status
    //     _display.setCursor(0, 50);
    //     _display.printf("PWM: %d-%d %s", debugInfo.currentMinPWM, debugInfo.currentMaxPWM,
    //                   debugInfo.inSafetyPause ? "SAFE" : "");

    //     // Copy display buffer to staging buffer
    //     uint8_t* displayBuffer = _display.getBuffer();
    //     memcpy(_stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    // }
    // _display.clearDisplay();

    // // Get current color sensor data
    // const ColorData& colorData = SensorDataBuffer::get_instance().getLatestColorData();

    // // Title
    // drawCenteredText("RGB Sensor Test", 0, 1);

    // if (colorData.isValid) {
    //     // RGB values
    //     _display.setCursor(0, 15);
    //     _display.printf("R: %3d", colorData.redValue);

    //     _display.setCursor(0, 25);
    //     _display.printf("G: %3d", colorData.greenValue);

    //     _display.setCursor(0, 35);
    //     _display.printf("B: %3d", colorData.blueValue);

    //     // Show detected color
    //     ColorTypes::ColorType detectedColor = SensorDataBuffer::get_instance().classifyCurrentColor();
    //     _display.setCursor(0, 50);
    //     _display.print("Color: ");
    //     switch (detectedColor) {
    //         case ColorTypes::COLOR_RED:   _display.print("RED"); break;
    //         case ColorTypes::COLOR_GREEN: _display.print("GREEN"); break;
    //         case ColorTypes::COLOR_BLUE:  _display.print("BLUE"); break;
    //         case ColorTypes::COLOR_WHITE: _display.print("WHITE"); break;
    //         case ColorTypes::COLOR_BLACK: _display.print("BLACK"); break;
    //         case ColorTypes::COLOR_YELLOW: _display.print("YELLOW"); break;
    //         case ColorTypes::COLOR_NONE:  _display.print("NONE"); break;
    //     }
    // } else {
    //     _display.setCursor(0, 20);
    //     _display.print("No color data");
    // }

    // // Copy display buffer to staging buffer
    // uint8_t* displayBuffer = _display.getBuffer();
    // memcpy(_stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    // return;
    if (career_quest_triggers.isS3P3Active()) {
        career_quest_triggers.render_s3_p3_animation();
        // Copy display buffer to staging buffer
        uint8_t* display_buffer = instance._display.getBuffer();
        memcpy(instance._stagingBuffer, display_buffer, DISPLAY_BUFFER_SIZE);
    } else if (!instance._customScreenActive) {
        instance._display.clearDisplay();

        // Draw company name (smaller)
        draw_centered_text("Lever Labs", 2, 2);

        // Show PipID below circle if WebSocket connected
        if (WebSocketManager::get_instance().is_ws_connected() && !WebSocketManager::get_instance().is_user_connected_to_this_pip()) {
            String pip_id = PreferencesManager::get_instance().get_pip_id();
            draw_centered_text(pip_id, 30, 3);
        } else if (WebSocketManager::get_instance().is_ws_connected() && WebSocketManager::get_instance().is_user_connected_to_this_pip()) {
            draw_centered_text("Connected!", 30, 2);
        } else {
            instance._display.fillCircle(instance._display.width() / 2, 40, 10, SSD1306_WHITE);
        }

        // Copy display buffer to staging buffer
        uint8_t* display_buffer = instance._display.getBuffer();
        memcpy(instance._stagingBuffer, display_buffer, DISPLAY_BUFFER_SIZE);
    }
}

bool DisplayScreen::has_content_changed() {
    DisplayScreen& instance = DisplayScreen::get_instance();
    return memcmp(instance._stagingBuffer, instance._currentDisplayBuffer, DISPLAY_BUFFER_SIZE) != 0;
}

void DisplayScreen::copy_current_buffer() {
    DisplayScreen& instance = DisplayScreen::get_instance();
    memcpy(instance._currentDisplayBuffer, instance._stagingBuffer, DISPLAY_BUFFER_SIZE);
}

float DisplayScreen::get_display_update_rate() {
    DisplayScreen& instance = DisplayScreen::get_instance();
    uint32_t elapsed = millis() - instance._perfStartTime;
    if (elapsed == 0) {
        return 0.0;
    }
    return (float)instance._displayUpdates * 1000.0 / (float)elapsed;
}

void DisplayScreen::reset_performance_counters() {
    DisplayScreen& instance = DisplayScreen::get_instance();
    instance._displayUpdates = 0;
    instance._contentGenerations = 0;
    instance._skippedUpdates = 0;
    instance._perfStartTime = millis();
}

void DisplayScreen::turn_display_off() {
    DisplayScreen& instance = DisplayScreen::get_instance();
    if (!instance._initialized) {
        return;
    }

    instance._displayOff = true;
    instance._customScreenActive = false;
    instance._isShowingStartScreen = false;

    instance._display.clearDisplay();
    instance._display.display();

    SerialQueueManager::get_instance().queue_message("Display turned off");
}

void DisplayScreen::turn_display_on() {
    DisplayScreen& instance = DisplayScreen::get_instance();
    if (!instance._initialized) {
        return;
    }

    instance._displayOff = false;
    SerialQueueManager::get_instance().queue_message("Display turned on");

    // Update buffer tracking
    memset(instance._stagingBuffer, 0, DISPLAY_BUFFER_SIZE);
    copy_current_buffer();
    instance._displayUpdates++;
}
