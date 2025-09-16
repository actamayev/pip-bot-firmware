// #include "straight_line_drive.h"

// constexpr float StraightLineDrive::KP_COUNTS_TO_PWM;
// constexpr int16_t StraightLineDrive::MAX_CORRECTION_PWM;
// constexpr int16_t StraightLineDrive::MIN_FORWARD_SPEED;

// void StraightLineDrive::enable() {
//     _straightDrivingEnabled = true;
    
//     // Get current encoder counts as our baseline
//     auto currentCounts = SensorDataBuffer::getInstance().getLatestEncoderCounts();
//     _initialLeftCount = currentCounts.first;
//     _initialRightCount = currentCounts.second;
    
//     SerialQueueManager::getInstance().queueMessage("StraightLineDrive enabled");
// }

// void StraightLineDrive::disable() {
//     _straightDrivingEnabled = false;
//     SerialQueueManager::getInstance().queueMessage("StraightLineDrive disabled");
// }

// void StraightLineDrive::update(int16_t& leftSpeed, int16_t& rightSpeed) {
//     if (!_straightDrivingEnabled) return;

//     // Only apply corrections if both motors are moving forward in the same direction
//     if (!(leftSpeed > 0 && rightSpeed > 0)) return;

//     // Get current encoder counts
//     auto currentCounts = SensorDataBuffer::getInstance().getLatestEncoderCounts();
//     int64_t currentLeftCount = currentCounts.first;
//     int64_t currentRightCount = currentCounts.second;
    
//     // Calculate travel since enable (absolute values for distance comparison)
//     int64_t leftTravel = abs(currentLeftCount - _initialLeftCount);
//     int64_t rightTravel = abs(currentRightCount - _initialRightCount);
    
//     // Calculate count error (positive = left wheel ahead, negative = right wheel ahead)
//     int64_t countError = leftTravel - rightTravel;
    
//     // Update debug info
//     _debugInfo.leftCounts = leftTravel;
//     _debugInfo.rightCounts = rightTravel;
//     _debugInfo.countError = countError;
    
//     // Calculate proportional correction
//     int16_t correction = static_cast<int16_t>(KP_COUNTS_TO_PWM * static_cast<float>(countError));
    
//     // Limit correction magnitude
//     correction = constrain(correction, -MAX_CORRECTION_PWM, MAX_CORRECTION_PWM);
    
//     // Store original speeds
//     int16_t originalLeftSpeed = leftSpeed;
//     int16_t originalRightSpeed = rightSpeed;
    
//     // Apply correction by reducing speed of faster wheel (SLD approach)
//     if (correction > 0) {
//         // Left wheel is ahead, slow it down
//         leftSpeed = originalLeftSpeed - abs(correction);
//         rightSpeed = originalRightSpeed;  // Keep right speed unchanged
//     } else if (correction < 0) {
//         // Right wheel is ahead, slow it down
//         leftSpeed = originalLeftSpeed;    // Keep left speed unchanged
//         rightSpeed = originalRightSpeed - abs(correction);
//     }
    
//     // Ensure minimum forward speed to prevent stopping
//     leftSpeed = max(leftSpeed, MIN_FORWARD_SPEED);
//     rightSpeed = max(rightSpeed, MIN_FORWARD_SPEED);
    
//     // Constrain to valid motor speed range
//     leftSpeed = constrain(leftSpeed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
//     rightSpeed = constrain(rightSpeed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
    
//     // Update debug info with final speeds and correction
//     _debugInfo.leftSpeed = leftSpeed;
//     _debugInfo.rightSpeed = rightSpeed;
//     _debugInfo.correction = correction;
// }
