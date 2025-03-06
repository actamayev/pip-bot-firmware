#include "./display_screen.h"

// Initialize the display with explicit Wire reference
bool DisplayScreen::init(TwoWire& wire) {
    if (initialized) return true;  // Already initialized
    
    // Initialize the OLED display with the provided Wire instance
    display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &wire, OLED_RESET);
    
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        return false;
    }

    initialized = true;
    
    // Clear the buffer
    display.clearDisplay();
    
    // Start the startup sequence
    startStartupSequence();
    
    Serial.println("OLED display initialized successfully!");
    return true;
}

// Main update method - call this in the task loop
void DisplayScreen::update() {
    if (!initialized) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    // Always process startup animation if active
    if (startupActive) {
        updateStartupAnimation();
        return;
    }
    
    // For regular display updates, use frame rate control
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        updateRegularDisplay();
    }
}

// Update the startup animation
void DisplayScreen::updateStartupAnimation() {
    unsigned long currentTime = millis();
    
    // Calculate elapsed time (handle millis() overflow)
    unsigned long elapsedTime = (currentTime >= startupStartTime) ? 
        (currentTime - startupStartTime) : 
        (UINT32_MAX - startupStartTime + currentTime);
        
    if (elapsedTime >= STARTUP_DURATION) {
        // Startup animation is complete
        startupActive = false;
        startupComplete = true;
        return;
    }
    
    // Animate startup screen
    clear();
    
    // Draw border
    display.drawRect(0, 0, display.width(), display.height(), SSD1306_WHITE);
    
    // Draw company name and product
    drawCenteredText("Blue Dot Robots", 10, 1);
    drawCenteredText("Pip", 35, 1);
    
    // Calculate car position based on elapsed time
    // Move from left (-20) to right (SCREEN_WIDTH + 20)
    float progress = (float)elapsedTime / STARTUP_DURATION;
    carPosition = -20 + progress * (SCREEN_WIDTH + 40);
    
    // Draw the car - simple car shape
    int carY = 50;
    display.fillRect(carPosition, carY, 15, 5, SSD1306_WHITE);       // Car body
    display.fillRect(carPosition + 3, carY - 3, 9, 3, SSD1306_WHITE); // Car top
    display.fillCircle(carPosition + 3, carY + 5, 2, SSD1306_WHITE);  // Left wheel
    display.fillCircle(carPosition + 12, carY + 5, 2, SSD1306_WHITE); // Right wheel
    
    renderDisplay();
}

// Update the regular display content (after startup)
void DisplayScreen::updateRegularDisplay() {
    // Clear the display
    clear();
    
    // Show different content based on the display mode
    // Cycle through modes every 2 seconds (40 frames at 50ms interval)
    displayMode = (animationCounter / 40) % 4;
    
    switch(displayMode) {
        case 0:
            // Show company info
            drawCenteredText("Blue Dot Robots", 5, 1);
            drawText("Pip Status", 10, 30, 1);
            drawText("Mode: " + String(displayMode), 10, 45, 1);
            break;

        case 1:
            // Show a progress bar (could be battery level)
            drawCenteredText("Battery Level", 5, 1);
            drawProgressBar(animationCounter % 101, 25);
            break;
            
        case 2:
            // Show some sensor data (replace with actual sensor readings)
            drawCenteredText("Sensor Data", 5, 1);
            drawText("Temp: " + String(random(20, 30)) + " C", 10, 25, 1);
            drawText("Speed: " + String(random(0, 15)) + " m/s", 10, 40, 1);
            break;
            
        case 3:
            // Animation
            drawCenteredText("Status", 5, 1);
            // Draw a moving dot
            int pos = animationCounter % SCREEN_WIDTH;
            // Use sine wave for vertical position
            float angle = (float)animationCounter / 10.0;
            int height = 35 + sin(angle) * 10;
            
            display.fillCircle(pos, height, 3, SSD1306_WHITE);
            break;
    }
    
    // Update the counter
    animationCounter++;
    
    // Render the display
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
void DisplayScreen::drawText(const String& text, int x, int y, int size) {
    if (!initialized) return;
    display.setTextSize(size);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x, y);
    display.println(text);
}

// Draw centered text
void DisplayScreen::drawCenteredText(const String& text, int y, int size) {
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

// Draw a progress bar
void DisplayScreen::drawProgressBar(int progress, int y) {
    if (!initialized) return;
    // progress should be 0-100
    progress = constrain(progress, 0, 100);
    
    // Draw the progress bar border
    display.drawRect(0, y, SCREEN_WIDTH, 10, SSD1306_WHITE);
    
    // Fill the progress bar
    int fillWidth = (progress * (SCREEN_WIDTH - 4)) / 100;
    display.fillRect(2, y + 2, fillWidth, 6, SSD1306_WHITE);
    
    // Display the percentage
    drawCenteredText(String(progress) + "%", y + 20, 1);
}

// Start the startup animation sequence
void DisplayScreen::startStartupSequence() {
    if (!initialized) return;
    startupActive = true;
    startupComplete = false;
    startupStartTime = millis();
    carPosition = -20;  // Start the car off-screen
}
