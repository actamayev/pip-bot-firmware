#include "display_screen.h"

// Initialize the display with explicit Wire reference
bool DisplayScreen::init(bool showStartup) {
    if (initialized) return true;  // Already initialized

    display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        SerialQueueManager::getInstance().queueMessage("SSD1306 allocation failed");
        return false;
    }

    initialized = true;

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
    
    // Only update display at regular intervals to save processing
    if (currentTime - lastUpdateTime < UPDATE_INTERVAL) return;
    lastUpdateTime = currentTime;
    
    // If nothing else has been set to display, keep the start screen showing
    if (!customScreenActive) {
        showStartScreen();
    }
}

// Show the start screen
void DisplayScreen::showStartScreen() {
    if (!initialized || isShowingStartScreen) return;

    turnDisplayOff();
    
    // Draw border
    display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    
    // Draw company name (smaller)
    drawCenteredText("Blue Dot Robots", 1);
    
    // Draw circle underneath
    display.fillCircle(display.width()/2, 40, 10, SSD1306_WHITE);
    
    renderDisplay();
    isShowingStartScreen = true;
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

void DisplayScreen::showLowBatteryScreen() {
    if (!initialized) return;

    turnDisplayOff();
    
    // Draw border
    display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    
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
    
    renderDisplay();
}

void DisplayScreen::turnDisplayOff() {
    if (!initialized) return;
    display.clearDisplay();
    display.display();
}
