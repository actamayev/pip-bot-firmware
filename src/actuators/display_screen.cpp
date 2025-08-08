#include "display_screen.h"

// Initialize the display with explicit Wire reference
bool DisplayScreen::init() {
    if (initialized) return true;  // Already initialized
    
    // Initialize the OLED display with the provided Wire instance
    // display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, Wire1);
    // 5/10/25: TODO: Change to:
    // display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        SerialQueueManager::getInstance().queueMessage("SSD1306 allocation failed");
        return false;
    }

    initialized = true;

    clear();

    // Start the startup sequence
    showStartScreen();
    
    SerialQueueManager::getInstance().queueMessage("OLED display initialized successfully!");
    return true;
}

// Main update method - call this in the task loop
void DisplayScreen::update() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    
    // Check if we're still in the initial screen display period
    if (isShowingStartScreen) {
        // If 2 seconds have passed, mark start screen as complete but keep displaying it
        if (currentTime - startScreenStartTime >= START_SCREEN_DURATION) {
            isShowingStartScreen = false;
            redrawStartScreen = true; // Redraw without animation when done
        } else {
            // During the startup animation, update the car position frequently
            if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) { // ~30fps for smooth animation
                lastUpdateTime = currentTime;
                showStartScreen(false); // Update animation without resetting timer
            }
            return;
        }
    }

    // Only update display at regular intervals to save processing
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        
        // If nothing else has been set to display, keep the start screen showing
        if (!customScreenActive) {
            // If we just finished showing the start screen animation, redraw it statically
            if (redrawStartScreen) {
                showStartScreen(false); // Show without resetting timer
                redrawStartScreen = false;
            }
        }
    }
}

// Show the start screen with car animation
void DisplayScreen::showStartScreen(bool resetTimer) {
    if (!initialized) return;
    
    if (resetTimer) {
        startScreenStartTime = millis();
        isShowingStartScreen = true;
        redrawStartScreen = false;
    }

    // For animation, calculate car position based on elapsed time
    unsigned long currentTime = millis();
    int16_t carPosition = -20; // Default position if not animating

    if (isShowingStartScreen) {
        // Calculate elapsed time (handle millis() overflow)
        unsigned long elapsedTime = (currentTime >= startScreenStartTime) ? 
            (currentTime - startScreenStartTime) : 
            (UINT32_MAX - startScreenStartTime + currentTime);
            
        // Calculate car position based on elapsed time
        // Move from left (-20) to right (SCREEN_WIDTH + 20)
        float progress = (float)elapsedTime / START_SCREEN_DURATION;
        carPosition = -20 + progress * (SCREEN_WIDTH + 40);
    }

    clear();
    
    // Draw border
    display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    
    // Draw company name (smaller)
    drawCenteredText("Blue Dot Robots", 1);
    
    // Draw circle underneath
    display.fillCircle(display.width()/2, 40, 10, SSD1306_WHITE);

    // Draw the car - simple car shape
    const uint8_t carY = 55;
    display.fillRect(carPosition, carY, 15, 5, SSD1306_WHITE);       // Car body
    display.fillRect(carPosition + 3, carY - 3, 9, 3, SSD1306_WHITE); // Car top
    display.fillCircle(carPosition + 3, carY + 5, 2, SSD1306_WHITE);  // Left wheel
    display.fillCircle(carPosition + 12, carY + 5, 2, SSD1306_WHITE); // Right wheel
    
    renderDisplay();
}

// Show ToF sensor distances
void DisplayScreen::showDistanceSensors(SideTofCounts sideTofCounts) {
    // If we're still showing the start screen, don't show anything yet
    if (!initialized || isShowingStartScreen) return;

    customScreenActive = true;

    clear();
    
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

// Clear the display
void DisplayScreen::clear() {
    if (!initialized) return;
    display.clearDisplay();
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
    isShowingStartScreen = false;
    
    // Clear display
    clear();
    
    // Copy the buffer directly to the display
    // The buffer is already in SSD1306 format from React
    display.getBuffer(); // Get the internal buffer pointer if needed
    
    // Or use drawBitmap if available:
    // display.drawBitmap(0, 0, buffer, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    
    // Alternative: Write directly to display buffer
    memcpy(display.getBuffer(), buffer, 1024);
    
    renderDisplay();
    
    SerialQueueManager::getInstance().queueMessage("Custom display buffer applied");
}

void DisplayScreen::turnScreenOff() {
    if (!initialized) return;
    SerialQueueManager::getInstance().queueMessage("Turning screen off");
    display.clearDisplay();
    display.display();
}
