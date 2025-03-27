#include "../utils/config.h"
#include "./encoder_manager.h"

EncoderManager encoderManager;

EncoderManager::EncoderManager() {
    _leftWheelRPM = 0;
    _rightWheelRPM = 0;
    _lastUpdateTime = 0;
    Serial.println("Creating encoder manager");

    // Initialize ESP32Encoder library
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    
    // Setup left encoder
    _leftEncoder.attachHalfQuad(LEFT_MOTOR_ENCODER_A, LEFT_MOTOR_ENCODER_B);
    _leftEncoder.clearCount();
    
    // Setup right encoder
    _rightEncoder.attachHalfQuad(RIGHT_MOTOR_ENCODER_A, RIGHT_MOTOR_ENCODER_B);
    _rightEncoder.clearCount();
    
    _lastUpdateTime = millis();
}

void EncoderManager::update() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - _lastUpdateTime;
    
    // Debug pulse counts
    int64_t leftPulses = _leftEncoder.getCount();
    int64_t rightPulses = _rightEncoder.getCount();
    
    // Only update if enough time has passed
    if (elapsedTime >= RPM_CALC_INTERVAL) {
        // Calculate motor shaft RPM - NOTE: Using elapsedTime in seconds
        float leftMotorShaftRPM = (float)(leftPulses * 60) / (ENCODER_CPR * (elapsedTime / 1000.0));
        float rightMotorShaftRPM = (float)(rightPulses * 60) / (ENCODER_CPR * (elapsedTime / 1000.0));

        // Calculate wheel RPM
        _leftWheelRPM = leftMotorShaftRPM / GEAR_RATIO;
        _rightWheelRPM = rightMotorShaftRPM / GEAR_RATIO;

        // Reset pulse counters for next interval
        _leftEncoder.clearCount();
        _rightEncoder.clearCount();

        _lastUpdateTime = currentTime;
    }
}

WheelRPMs EncoderManager::getBothWheelRPMs() {
    update();  // Update once for both wheels
    return {
        leftWheelRPM: _leftWheelRPM,
        rightWheelRPM: _rightWheelRPM
    };
}

void EncoderManager::log_motor_rpm() {
    // Return if logging is disabled
    if (!should_log_motor_rpm) return;
    
    // Check if 10ms has elapsed since last log
    unsigned long currentTime = millis();
    if (currentTime - _lastLogTime < 10) return;
    
    // Get latest RPM values
    auto rpms = getBothWheelRPMs();

    // Log the values
    Serial.printf("Left wheel RPM: %.2f\n", rpms.leftWheelRPM);
    Serial.printf("Right wheel RPM: %.2f\n", rpms.rightWheelRPM);
    
    // Update last log time
    _lastLogTime = currentTime;
}

void EncoderManager::initNetworkSelection() {
    // Reset encoder position
    _rightEncoder.clearCount();
    _lastRightEncoderValue = 0;
    _lastHapticPosition = 0;  // Also reset the haptic position
    _networkSelectionActive = true;
    Serial.println("Network selection mode activated");
    Serial.println("Turn right encoder to scroll through networks");
}

void EncoderManager::updateNetworkSelection() {
    if (!_networkSelectionActive) return;
    
    // Process timing logic
    unsigned long currentTime = millis();
    
    // Check if we're in a cooldown period
    if (!_scrollingEnabled) {
        // Wait for the cooldown to finish
        if (currentTime - _scrollCooldownTime >= _scrollCooldownDuration) {
            _scrollingEnabled = true;
            // Reset encoder position once cooldown is complete
            _rightEncoder.clearCount();
            _lastRightEncoderValue = 0;
            _lastHapticPosition = 0;
            Serial.println("Scrolling re-enabled after cooldown");
        } else {
            // We're still in cooldown, don't process scrolling
            return;
        }
    }

    // Don't process encoder changes if haptic feedback is in progress
    if (motorDriver.isHapticInProgress()) return;
    
    // Get current encoder value
    int32_t currentValue = _rightEncoder.getCount();
    
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
        uint8_t hapticStrength = wrappedAround ? 255 : 255;
        uint8_t hapticDuration = wrappedAround ? 28 : 15;

        // Update the selection
        wifiManager.setSelectedNetworkIndex(newIndex);
        
        // Print updated network list
        wifiManager.printNetworkList(wifiManager.getAvailableNetworks());
        
        // Store current encoder value BEFORE starting haptic feedback
        // This way we don't get false readings during haptic feedback
        _lastRightEncoderValue = currentValue;
        _lastHapticPosition = currentValue;
        
        // Apply haptic feedback
        motorDriver.start_haptic_feedback(direction, hapticStrength, hapticDuration);
        
        // Start cooldown period
        _scrollingEnabled = false;
        _scrollCooldownTime = currentTime;
        _selectionChanged = true;
        
        Serial.printf("Network selection changed to index %d, entering cooldown for %d ms\n", 
                     newIndex, _scrollCooldownDuration);
    }
}

void EncoderManager::processNetworkSelection() {
    if (!_networkSelectionActive) return;
    updateNetworkSelection();
}
