#include "straight_line_drive.h"

constexpr int16_t StraightLineDrive::MAX_CORRECTION_PWM;
constexpr int16_t StraightLineDrive::MIN_FORWARD_SPEED;
constexpr int16_t StraightLineDrive::SPEED_SCALE_THRESHOLD;

void StraightLineDrive::enable() {
    _straightDrivingEnabled = true;
    
    // Get current yaw heading as our baseline
    _initialHeading = -SensorDataBuffer::getInstance().getLatestYaw();  // Note: negative for consistency with turning manager
    
    // Initialize debug info
    _debugInfo.initialHeading = _initialHeading;
    _smoothedHeading = _initialHeading;

    SerialQueueManager::getInstance().queueMessage("StraightLineDrive enabled (IMU-based)");
}

void StraightLineDrive::disable() {
    _straightDrivingEnabled = false;
    SerialQueueManager::getInstance().queueMessage("StraightLineDrive disabled");
}

void StraightLineDrive::update(int16_t& leftSpeed, int16_t& rightSpeed) {
    if (!_straightDrivingEnabled) return;

    // Only apply corrections if both motors are moving forward in the same direction
    if (!(leftSpeed > 0 && rightSpeed > 0)) return;

    // Get current yaw heading and apply smoothing
    float rawHeading = -SensorDataBuffer::getInstance().getLatestYaw();
    _smoothedHeading = HEADING_SMOOTH_FACTOR * _smoothedHeading + (1.0f - HEADING_SMOOTH_FACTOR) * rawHeading;
    
    // Calculate heading error with wrap-around handling
    float headingError = calculateHeadingError(_smoothedHeading, _initialHeading);
    
    // Apply dead zone - skip tiny corrections that cause oscillation
    if (abs(headingError) < DEAD_ZONE_DEGREES) {
        return; // No correction needed for small errors
    }
    
    // Update debug info
    _debugInfo.currentHeading = _smoothedHeading;
    _debugInfo.headingError = headingError;
    
    // Calculate speed-adaptive correction scaling
    float correctionScale = calculateCorrectionScale(leftSpeed, rightSpeed);
    _debugInfo.correctionScale = correctionScale;
    
    // Calculate proportional correction with speed scaling
    int16_t correction = static_cast<int16_t>(KP_HEADING_TO_PWM * headingError * correctionScale);
    
    // Limit correction magnitude
    correction = constrain(correction, -MAX_CORRECTION_PWM, MAX_CORRECTION_PWM);
    
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

float StraightLineDrive::calculateCorrectionScale(int16_t leftSpeed, int16_t rightSpeed) {
    // Calculate average speed for scaling
    float avgSpeed = (abs(leftSpeed) + abs(rightSpeed)) / 2.0f;
    
    // Scale correction strength based on speed
    // Lower speeds get smaller corrections to avoid overcorrection
    // Higher speeds get larger corrections to handle higher momentum
    float scale;
    
    if (avgSpeed < SPEED_SCALE_THRESHOLD) {
        // Linear scaling from MIN_CORRECTION_SCALE to 1.0 for low speeds
        scale = MIN_CORRECTION_SCALE + 
                (1.0f - MIN_CORRECTION_SCALE) * (avgSpeed / SPEED_SCALE_THRESHOLD);
    } else {
        // Linear scaling from 1.0 to MAX_CORRECTION_SCALE for high speeds  
        float highSpeedRange = MAX_MOTOR_SPEED - SPEED_SCALE_THRESHOLD;
        float speedAboveThreshold = avgSpeed - SPEED_SCALE_THRESHOLD;
        scale = 1.0f + (MAX_CORRECTION_SCALE - 1.0f) * (speedAboveThreshold / highSpeedRange);
    }
    
    // Ensure scale stays within bounds
    return constrain(scale, MIN_CORRECTION_SCALE, MAX_CORRECTION_SCALE);
}
