#include "straight_line_drive.h"

constexpr int16_t StraightLineDrive::MIN_FORWARD_SPEED;

void StraightLineDrive::enable() {
    _straightDrivingEnabled = true;

    // Get current yaw heading as our baseline
    _initialHeading = -SensorDataBuffer::getInstance().getLatestYaw();  // Note: negative for consistency with turning manager

    // Initialize debug info
    _debugInfo.initialHeading = _initialHeading;

    SerialQueueManager::getInstance().queueMessage("StraightLineDrive enabled (IMU-based)");
}

void StraightLineDrive::disable() {
    _straightDrivingEnabled = false;

    // Reset all member variables to clean state
    _initialHeading = 0.0f;
    // _debugInfo = DebugInfo{}; // Reset debug info to default values

    SerialQueueManager::getInstance().queueMessage("StraightLineDrive disabled");
}

void StraightLineDrive::update(int16_t& leftSpeed, int16_t& rightSpeed) {
    if (!_straightDrivingEnabled) return;

    // Only apply corrections if both motors are moving forward in the same direction
    if (!(leftSpeed > 0 && rightSpeed > 0)) return;

    // Get current yaw heading directly (no smoothing)
    float currentHeading = -SensorDataBuffer::getInstance().getLatestYaw();

    // Calculate heading error with wrap-around handling
    float headingError = calculateHeadingError(currentHeading, _initialHeading);
    
    // Update debug info
    _debugInfo.currentHeading = currentHeading;
    _debugInfo.headingError = headingError;

    // Apply dead zone - skip tiny corrections that cause oscillation
    if (abs(headingError) < DEAD_ZONE_DEGREES) return; // No correction needed for small errors

    // Calculate proportional correction (simplified)
    int16_t correction = static_cast<int16_t>(KP_HEADING_TO_PWM * headingError);
    
    // Store original speeds
    int16_t originalLeftSpeed = leftSpeed;
    int16_t originalRightSpeed = rightSpeed;
    
    // Apply correction based on heading error
    if (correction > 0) {
        // Positive heading error = robot drifted clockwise (right) = slow down left wheel to correct counter-clockwise
        leftSpeed = originalLeftSpeed - abs(correction);
        rightSpeed = originalRightSpeed;   // Keep right speed unchanged
    } else if (correction < 0) {
        // Negative heading error = robot drifted counter-clockwise (left) = slow down right wheel to correct clockwise
        leftSpeed = originalLeftSpeed;    // Keep left speed unchanged
        rightSpeed = originalRightSpeed - abs(correction);
    }
    
    // Ensure minimum forward speed to prevent stopping
    leftSpeed = max(leftSpeed, MIN_FORWARD_SPEED);
    rightSpeed = max(rightSpeed, MIN_FORWARD_SPEED);
    
    // Constrain to valid motor speed range
    leftSpeed = constrain(leftSpeed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    rightSpeed = constrain(rightSpeed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    
    // Update debug info with final speeds and correction
    _debugInfo.leftSpeed = leftSpeed;
    _debugInfo.rightSpeed = rightSpeed;
    _debugInfo.correction = correction;
}

float StraightLineDrive::calculateHeadingError(float currentHeading, float targetHeading) {
    float error = currentHeading - targetHeading;
    
    // Handle wrap-around using shortest path (same logic as turning manager)
    while (error > 180.0f) error -= 360.0f;
    while (error < -180.0f) error += 360.0f;
    
    return error;
}
