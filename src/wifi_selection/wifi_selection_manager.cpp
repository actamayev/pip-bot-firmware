#include "../utils/config.h"
#include "wifi_selection_manager.h"

void WifiSelectionManager::initNetworkSelection() {
    // Reset encoder position
    encoderManager._rightEncoder.clearCount();
    _lastRightEncoderValue = 0;
    _networkSelectionActive = true;
    Serial.println("Network selection mode activated");
    Serial.println("Turn right encoder to scroll through networks");
}

void WifiSelectionManager::updateNetworkSelection() {
    if (!_networkSelectionActive) return;
    
    // Process timing logic
    unsigned long currentTime = millis();
    
    // Check if we're in a cooldown period
    if (!_scrollingEnabled) {
        // Wait for the cooldown to finish
        if (currentTime - _scrollCooldownTime < _scrollCooldownDuration) {
            // We're still in cooldown, don't process scrolling
            return;
        } 
        _scrollingEnabled = true;
        // Reset encoder position once cooldown is complete
        encoderManager._rightEncoder.clearCount();
        _lastRightEncoderValue = 0;
        Serial.println("Scrolling re-enabled after cooldown");
    }

    // Get current encoder value
    int32_t currentValue = encoderManager._rightEncoder.getCount();
    
    // Calculate delta (how much the encoder has moved)
    int32_t encoderDelta = currentValue - _lastRightEncoderValue;
    
    // Ignore very small movements to reduce noise
    if (abs(encoderDelta) < 2) return;
    
    // Only proceed if encoder has moved enough to trigger a scroll
    if (abs(encoderDelta) >= _scrollSensitivity) {
        // Calculate direction for simple movement (positive or negative)
        int direction = (encoderDelta > 0) ? 1 : -1;
        
        // Calculate number of steps (limit to 1 to prevent skipping)
        int steps = direction; // Always just move 1 item at a time
        
        // Update the WiFi manager's selected network index
        WiFiManager& wifiManager = WiFiManager::getInstance();
        int currentIndex = wifiManager.getSelectedNetworkIndex();
        int newIndex = currentIndex + steps;
        
        int maxIndex = wifiManager.getAvailableNetworks().size() - 1;
        bool wrappedAround = false;
        
        // Handle wrapping with boundary feedback
        if (newIndex < 0) {
            newIndex = maxIndex; // Wrap to end
            wrappedAround = true;
        } else if (newIndex > maxIndex) {
            newIndex = 0; // Wrap to beginning
            wrappedAround = true;
        }

        // Determine haptic strength based on wrap-around
        uint8_t hapticStrength = wrappedAround ? 225 : 225;
        uint8_t hapticDuration = wrappedAround ? 30 : 20;

        // Update the selection
        wifiManager.setSelectedNetworkIndex(newIndex);

        // Print updated network list
        wifiManager.printNetworkList(wifiManager.getAvailableNetworks());
        
        // Store current encoder value BEFORE starting haptic feedback
        
        // Apply haptic feedback
        HapticFeedbackManager::getInstance().startFeedback(direction, hapticStrength, hapticDuration);
        
        // Start cooldown period
        _scrollingEnabled = false;
        _scrollCooldownTime = currentTime;
        // This way we don't get false readings during haptic feedback
        _lastRightEncoderValue = currentValue;

        Serial.printf("Network selection changed to index %d, entering cooldown for %d ms\n", 
                     newIndex, _scrollCooldownDuration);
    }
}

void WifiSelectionManager::processNetworkSelection() {
    if (!_networkSelectionActive) return;
    updateNetworkSelection();
}
