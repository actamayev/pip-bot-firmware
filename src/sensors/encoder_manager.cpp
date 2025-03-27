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

    // Get current encoder value
    int32_t currentValue = _rightEncoder.getCount();

    // Calculate deltas
    int32_t encoderDelta = currentValue - _lastRightEncoderValue;
    int32_t hapticDelta = currentValue - _lastHapticPosition;

    // First check if we should provide haptic feedback (more frequent)
    if (abs(hapticDelta) >= _hapticSensitivity) {
        // Calculate haptic direction
        int hapticDirection = (hapticDelta > 0) ? 1 : -1;
        
        // Provide haptic feedback (but don't change selection yet)
        if (abs(hapticDelta) >= _scrollSensitivity) {
            // If we're also about to change selection, use stronger feedback
            motorDriver.start_haptic_feedback(hapticDirection, 130, 15);
        } else {
            // Just haptic feedback, lighter
            motorDriver.start_haptic_feedback(hapticDirection, 90, 12);
        }
        
        // Update last haptic position (round to nearest haptic step to prevent drift)
        _lastHapticPosition = currentValue - (currentValue % _hapticSensitivity);
    }
    
    // Check if we've moved enough to change the network selection
    if (abs(encoderDelta) >= _scrollSensitivity) {
        // Calculate direction and number of steps
        int steps = encoderDelta / _scrollSensitivity;
        
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
        
        // Only handle wrap-around feedback here (regular feedback handled above)
        if (wrappedAround) {
            motorDriver.start_haptic_feedback(steps > 0 ? 1 : -1, 180, 20);
        }
        
        // Update the selection
        wifiManager.setSelectedNetworkIndex(newIndex);
        wifiManager.printNetworkList(wifiManager.getAvailableNetworks());
        
        // Store current value as last value (round to nearest scroll step to prevent drift)
        _lastRightEncoderValue = currentValue - (currentValue % _scrollSensitivity);
    }
}

void EncoderManager::processNetworkSelection() {
    if (!_networkSelectionActive) return;
    updateNetworkSelection();
}
