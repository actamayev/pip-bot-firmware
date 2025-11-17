#include "straight_line_drive.h"

#include <math.h>

#include <cmath>

constexpr int16_t StraightLineDrive::MIN_FORWARD_SPEED;

void StraightLineDrive::enable() {
    _straightDrivingEnabled = true;

    // Get current yaw heading as our baseline
    _initialHeading = -SensorDataBuffer::get_instance().get_latest_yaw(); // Note: negative for consistency with turning manager

    // Initialize debug info
    _debugInfo.initialHeading = _initialHeading;

    SerialQueueManager::get_instance().queue_message("StraightLineDrive enabled (IMU-based)");
}

void StraightLineDrive::disable() {
    if (!_straightDrivingEnabled) {
        return;
    }

    _straightDrivingEnabled = false;

    // Reset all member variables to clean state
    _initialHeading = 0.0f;
    _debugInfo = DebugInfo{}; // Reset debug info to default values

    SerialQueueManager::get_instance().queue_message("StraightLineDrive disabled");
}

void StraightLineDrive::update(int16_t& left_speed, int16_t& right_speed) {
    if (!_straightDrivingEnabled) {
        return;
    }

    // Only apply corrections if both motors are moving forward in the same direction
    if (left_speed <= 0 || right_speed <= 0) {
        return;
    }

    // Get current yaw heading directly (no smoothing)
    float current_heading = -SensorDataBuffer::get_instance().get_latest_yaw();

    // Calculate heading error with wrap-around handling
    float heading_error = calculate_heading_error(current_heading, _initialHeading);

    // Update debug info
    _debugInfo.currentHeading = current_heading;
    _debugInfo.headingError = heading_error;

    // Apply dead zone - skip tiny corrections that cause oscillation
    if (abs(heading_error) < DEAD_ZONE_DEGREES) {
        return; // No correction needed for small errors
    }

    // Calculate proportional correction (simplified)
    auto correction = static_cast<int16_t>(KP_HEADING_TO_PWM * heading_error);

    // Store original speeds
    int16_t original_left_speed = left_speed;
    int16_t original_right_speed = right_speed;

    // Apply correction based on heading error
    if (correction > 0) {
        // Positive heading error = robot drifted clockwise (right) = slow down left wheel to correct counter-clockwise
        left_speed = original_left_speed - abs(correction);
        right_speed = original_right_speed; // Keep right speed unchanged
    } else if (correction < 0) {
        // Negative heading error = robot drifted counter-clockwise (left) = slow down right wheel to correct clockwise
        left_speed = original_left_speed; // Keep left speed unchanged
        right_speed = original_right_speed - abs(correction);
    }

    // Ensure minimum forward speed to prevent stopping
    left_speed = max(left_speed, MIN_FORWARD_SPEED);
    right_speed = max(right_speed, MIN_FORWARD_SPEED);

    // Constrain to valid motor speed range
    left_speed = constrain(left_speed, -MAX_MOTOR_PWM, MAX_MOTOR_PWM);
    right_speed = constrain(right_speed, -MAX_MOTOR_PWM, MAX_MOTOR_PWM);

    // Update debug info with final speeds and correction
    _debugInfo.leftSpeed = left_speed;
    _debugInfo.rightSpeed = right_speed;
    _debugInfo.correction = correction;
}

float StraightLineDrive::calculate_heading_error(float current_heading, float target_heading) {
    float error = current_heading - target_heading;

    // Handle wrap-around using shortest path (same logic as turning manager)
    while (error > 180.0f) {
        error -= 360.0f;
    }
    while (error < -180.0f) {
        error += 360.0f;
    }

    return error;
}
