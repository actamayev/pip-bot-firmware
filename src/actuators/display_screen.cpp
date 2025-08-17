#include "display_screen.h"

// Initialize the display with explicit Wire reference
bool DisplayScreen::init() {
    if (initialized) return true;  // Already initialized

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        SerialQueueManager::getInstance().queueMessage("SSD1306 allocation failed");
        return false;
    }

    initialized = true;

    turnDisplayOff();

    // Start the startup sequence
    showStartScreen(true);

    return true;
}

// Main update method - call this in the task loop
void DisplayScreen::update() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    
    // Only update display at regular intervals to save processing
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        
        // If nothing else has been set to display, keep the start screen showing
        if (!customScreenActive) {
            showStartScreen(false);
        }
    }
}

// Show the start screen
void DisplayScreen::showStartScreen(bool resetTimer) {
    if (!initialized) return;
    

    turnDisplayOff();
    
    // Draw border
    display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    
    // Draw company name (smaller)
    drawCenteredText("Blue Dot Robots", 1);
    
    // Draw circle underneath
    display.fillCircle(display.width()/2, 40, 10, SSD1306_WHITE);
    
    renderDisplay();
}

// Show ToF sensor distances
void DisplayScreen::showDistanceSensors(SideTofCounts sideTofCounts) {
    // If we're still showing the start screen, don't show anything yet
    if (!initialized) return;

    customScreenActive = true;

    turnDisplayOff();
    
    // Draw border
    display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    
    // Title
    drawCenteredText("Distance Sensors", 1);

    // Left sensor - use String(value) without decimal places
    drawText("Left:", 10, 25, 1);
    drawText(String(sideTofCounts.leftCounts) + " counts", 50, 25, 1);

    // Right sensor - use String(value) without decimal places
    drawText("Right:", 10, 40, 1);
    drawText(String(sideTofCounts.rightCounts) + " counts", 50, 40, 1);

    renderDisplay();
}

// Render the display (apply the buffer to the screen)
void DisplayScreen::renderDisplay() {
    if (!initialized) return;
    display.display();
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
    
    // Override any current display state
    customScreenActive = true;
    
    // Clear display
    turnDisplayOff();
    
    // Copy the buffer directly to the display
    // The buffer is already in SSD1306 format from React
    display.getBuffer(); // Get the internal buffer pointer if needed
    
    // Or use drawBitmap if available:
    // display.drawBitmap(0, 0, buffer, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    
    // Alternative: Write directly to display buffer
    memcpy(display.getBuffer(), buffer, 1024);
    
    renderDisplay();
}

void DisplayScreen::turnDisplayOff() {
    if (!initialized) return;
    display.clearDisplay();
    display.display();
}
