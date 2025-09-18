#include "display_screen.h"
#include "career_quest/career_quest_triggers.h"
#include "demos/straight_line_drive.h"
#include "demos/turning_manager.h"

// Initialize the display with explicit Wire reference
bool DisplayScreen::init(bool showStartup) {
    if (initialized) return true;  // Already initialized

    display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        SerialQueueManager::getInstance().queueMessage("SSD1306 allocation failed");
        return false;
    }

    initialized = true;
    perfStartTime = millis();
    
    // Initialize buffers
    memset(stagingBuffer, 0, DISPLAY_BUFFER_SIZE);
    memset(currentDisplayBuffer, 0, DISPLAY_BUFFER_SIZE);

    turnDisplayOff();

    // Conditionally start the startup sequence
    if (showStartup) {
        showStartScreen();
    }

    return true;
}

// Main update method - call this in the task loop
void DisplayScreen::update() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    
    // Only generate content at regular intervals
    if (currentTime - lastContentGeneration < UPDATE_INTERVAL) return;
    lastContentGeneration = currentTime;
    
    // Generate content to staging buffer
    generateContentToBuffer();
    contentGenerations++;
    
    // Check if content actually changed
    if (!hasContentChanged()) {
        // Content unchanged - skip expensive I2C operation
        skippedUpdates++;
        return;
    }
    // Content changed - update display (I2C operation)
    display.display();
    
    // Copy new content to current buffer for next comparison
    copyCurrentBuffer();
    
    displayUpdates++;
    lastDisplayUpdate = currentTime;
}

// Show the start screen
void DisplayScreen::showStartScreen() {
    if (!initialized || isShowingStartScreen) return;
    displayOff = false;

    display.clearDisplay();

    // Draw company name (smaller)
    drawCenteredText("Blue Dot Robots", 2, 1);

    // Draw circle underneath
    display.fillCircle(display.width()/2, 40, 10, SSD1306_WHITE);
    
    // Copy to staging buffer and force update
    uint8_t* displayBuffer = display.getBuffer();
    memcpy(stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    
    // Force display update for startup screen
    display.display();
    copyCurrentBuffer();
    
    isShowingStartScreen = true;
    customScreenActive = false;
}

// Render the display (apply the buffer to the screen)
void DisplayScreen::renderDisplay() {
    if (!initialized) return;
    
    // Force immediate display update (bypasses buffer comparison)
    display.display();
    
    // Update our buffer tracking
    uint8_t* displayBuffer = display.getBuffer();
    memcpy(stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    copyCurrentBuffer();
    
    displayUpdates++;
    lastDisplayUpdate = millis();
}

// Draw text at specified position
void DisplayScreen::drawText(const String& text, uint16_t x, uint16_t y, uint16_t size) {
    if (!initialized) return;
    display.setTextSize(size);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x, y);
    display.println(text);
}

// Draw centered text
void DisplayScreen::drawCenteredText(const String& text, uint16_t y, uint16_t size) {
    if (!initialized) return;
    int16_t x1, y1;
    uint16_t w, h;
    
    display.setTextSize(size);
    display.setTextColor(SSD1306_WHITE);
    
    // Calculate position for centered text
    display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, y);
    
    display.println(text);
}

// Add this method to your DisplayScreen class
void DisplayScreen::showCustomBuffer(const uint8_t* buffer) {
    if (!initialized) return;
    displayOff = false;

    // Override any current display state
    customScreenActive = true;
    isShowingStartScreen = false;
    
    // Clear display and copy buffer directly
    display.clearDisplay();
    memcpy(display.getBuffer(), buffer, DISPLAY_BUFFER_SIZE);
    
    // Use optimized render (will check for changes)
    uint8_t* displayBuffer = display.getBuffer();
    memcpy(stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    
    if (!hasContentChanged()) {
        skippedUpdates++;
        return;
    }
    display.display();
    copyCurrentBuffer();
    displayUpdates++;
}

void DisplayScreen::showLowBatteryScreen() {
    if (!initialized) return;

    customScreenActive = true;
    isShowingStartScreen = false;
    
    display.clearDisplay();
    
    // Draw warning icon (triangle with exclamation)
    int centerX = display.width() / 2;
    display.drawTriangle(centerX - 8, 20, centerX + 8, 20, centerX, 5, SSD1306_WHITE);
    display.drawPixel(centerX, 10, SSD1306_WHITE);
    display.drawPixel(centerX, 12, SSD1306_WHITE);
    display.drawPixel(centerX, 16, SSD1306_WHITE);
    
    // Draw "LOW BATTERY" text
    drawCenteredText("LOW BATTERY", 25, 1);
    
    // Draw "SHUTTING DOWN" text
    drawCenteredText("SHUTTING DOWN", 40, 1);
    
    // Force immediate display update for critical message
    renderDisplay();
}

void DisplayScreen::generateContentToBuffer() {
    // If display is off, don't generate any content
    if (displayOff) return;

    // if (StraightLineDrive::getInstance().isEnabled()) {
        // display.clearDisplay();
        
        // const auto& debugInfo = StraightLineDrive::getInstance().getDebugInfo();
        
        // // Title
        // drawCenteredText("Straight Line Drive", 0, 1);
        
        // // Motor speeds (int16_t -> %d)
        // display.setCursor(0, 10);
        // display.printf("L: %d  R: %d", debugInfo.leftSpeed, debugInfo.rightSpeed);
        
        // // Initial heading (float -> %.1f)
        // display.setCursor(0, 20);
        // display.printf("Init heading: %.1f", debugInfo.initialHeading);
        
        // // Current heading (float -> %.1f)
        // display.setCursor(0, 30);
        // display.printf("Curr heading: %.1f", debugInfo.currentHeading);

        // // Heading error (float -> %.1f)
        // display.setCursor(0, 40);
        // display.printf("Err: %.1f deg", debugInfo.headingError);
        
        // // Correction (int16_t -> %d)
        // display.setCursor(0, 50);
        // display.printf("Correction: %d", debugInfo.correction);
        
        // // Copy display buffer to staging buffer
        // uint8_t* displayBuffer = display.getBuffer();
        // memcpy(stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    // } else if (TurningManager::getInstance().isActive()) {
        display.clearDisplay();

        const auto& debugInfo = TurningManager::getInstance().getDebugInfo();

        // Title
        drawCenteredText("Turning Manager", 0, 1);

        // Target angle
        display.setCursor(0, 10);
        display.printf("Target: %.1f deg", debugInfo.targetAngle);

        // Cumulative rotation
        display.setCursor(0, 20);
        display.printf("Rotation: %.1f deg", debugInfo.cumulativeRotation);

        // Current error
        display.setCursor(0, 30);
        display.printf("Error: %.1f deg", debugInfo.currentError);

        // Velocity and PWM
        display.setCursor(0, 40);
        display.printf("Vel: %.1f d/s", debugInfo.currentVelocity);

        // PWM limits and status
        display.setCursor(0, 50);
        display.printf("PWM: %d-%d %s", debugInfo.currentMinPWM, debugInfo.currentMaxPWM,
                      debugInfo.inSafetyPause ? "SAFE" : "");

        // Copy display buffer to staging buffer
        uint8_t* displayBuffer = display.getBuffer();
        memcpy(stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    // }
    // if (careerQuestTriggers.isS3P3Active()) {
    //     careerQuestTriggers.renderS3P3Animation();
    //     // Copy display buffer to staging buffer
    //     uint8_t* displayBuffer = display.getBuffer();
    //     memcpy(stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    // } else if (!customScreenActive) {
    //     display.clearDisplay();
        
    //     // Draw company name (smaller)
    //     drawCenteredText("Blue Dot Robots", 2, 1.5);

    //     // Show PipID below circle if WebSocket connected
    //     if (
    //         WebSocketManager::getInstance().isWsConnected() &&
    //         !WebSocketManager::getInstance().isUserConnectedToThisPip()
    //     ) {
    //         String pipId = PreferencesManager::getInstance().getPipId();
    //         drawCenteredText(pipId, 30, 3);
    //     } else if (
    //         WebSocketManager::getInstance().isWsConnected() &&
    //         WebSocketManager::getInstance().isUserConnectedToThisPip()
    //     ) {
    //         drawCenteredText("Connected!", 30, 2);
    //     } else {
    //         display.fillCircle(display.width()/2, 40, 10, SSD1306_WHITE);
    //     }
        
    //     // Copy display buffer to staging buffer
    //     uint8_t* displayBuffer = display.getBuffer();
    //     memcpy(stagingBuffer, displayBuffer, DISPLAY_BUFFER_SIZE);
    // }
}

bool DisplayScreen::hasContentChanged() {
    return memcmp(stagingBuffer, currentDisplayBuffer, DISPLAY_BUFFER_SIZE) != 0;
}

void DisplayScreen::copyCurrentBuffer() {
    memcpy(currentDisplayBuffer, stagingBuffer, DISPLAY_BUFFER_SIZE);
}

float DisplayScreen::getDisplayUpdateRate() const {
    unsigned long elapsed = millis() - perfStartTime;
    if (elapsed == 0) return 0.0;
    return (float)displayUpdates * 1000.0 / (float)elapsed;
}

void DisplayScreen::resetPerformanceCounters() {
    displayUpdates = 0;
    contentGenerations = 0;
    skippedUpdates = 0;
    perfStartTime = millis();
}

void DisplayScreen::turnDisplayOff() {
    if (!initialized) return;
    
    displayOff = true;
    customScreenActive = false;
    isShowingStartScreen = false;
    
    display.clearDisplay();
    display.display();
    
    SerialQueueManager::getInstance().queueMessage("Display turned off");
}

void DisplayScreen::turnDisplayOn() {
    if (!initialized) return;
    
    displayOff = false;
    SerialQueueManager::getInstance().queueMessage("Display turned on");
    
    // Update buffer tracking
    memset(stagingBuffer, 0, DISPLAY_BUFFER_SIZE);
    copyCurrentBuffer();
    displayUpdates++;
}
