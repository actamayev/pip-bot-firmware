#include "haptic_feedback_manager.h"
#include "actuators/motor_driver.h"

void HapticFeedbackManager::startFeedback(int8_t direction, uint8_t strength, uint8_t duration_ms) {
    // Don't start new haptic feedback if we're already in a haptic sequence
    if (_hapticState != HAPTIC_IDLE) return;
    
    // Store direction for use in all stages
    _hapticDirection = direction;
    
    // Simplified sequence with more distinct feel
    // Calculate strengths as percentage of the input strength
    _resistanceStrength = min(255, (int)(strength * 1.2));  // Initial resistance
    _reversePulseStrength = min(255, (int)(strength * 0.9)); // Strong reverse pulse
    _centeringStrength = min(255, (int)(strength * 0.5));   // Gentle centering
    _finalBumpStrength = min(255, (int)(strength * 0.7));   // Definitive final "click"
    
    // Set durations for distinct feeling stages
    // Total duration will be longer than input duration to make effect more noticeable
    _resistanceDuration = min(255, (int)(duration_ms * 0.6)); // Initial resistance
    _reversePulseDuration = min(255, (int)(duration_ms * 0.3)); // Quick reverse pulse
    _centeringDuration = min(255, (int)(duration_ms * 0.8));  // Longer centering phase
    _finalBumpDuration = min(255, (int)(duration_ms * 0.4));  // Brief final bump
    _recoveryDuration = 50;  // Fixed recovery time before accepting new input
    
    // Start first stage - resistance against movement
    _hapticState = HAPTIC_RESISTANCE;
    _hapticStartTime = millis();
    
    // Apply resistance in the opposite direction of movement
    if (direction > 0) {
        motorDriver.right_motor_backward(_resistanceStrength);
    } else {
        motorDriver.right_motor_forward(_resistanceStrength);
    }
    
    // SerialQueueManager::getInstance().queueMessage("Haptic feedback: dir=%d, str=%d, dur=%d\n", 
    //              direction, strength, duration_ms);
}

void HapticFeedbackManager::update() {
    if (_hapticState == HAPTIC_IDLE) return;
    
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - _hapticStartTime;
    
    switch (_hapticState) {
        case HAPTIC_RESISTANCE:
            if (elapsedTime >= _resistanceDuration) {
                _hapticState = HAPTIC_REVERSE_PULSE;
                _hapticStartTime = currentTime;
                
                // Apply pulse in same direction as user movement
                if (_hapticDirection > 0) {
                    motorDriver.right_motor_forward(_reversePulseStrength);
                } else {
                    motorDriver.right_motor_backward(_reversePulseStrength);
                }
            }
            break;
            
        case HAPTIC_REVERSE_PULSE:
            if (elapsedTime >= _reversePulseDuration) {
                _hapticState = HAPTIC_CENTERING;
                _hapticStartTime = currentTime;
                
                // Apply gentle centering force
                if (_hapticDirection > 0) {
                    motorDriver.right_motor_backward(_centeringStrength);
                } else {
                    motorDriver.right_motor_forward(_centeringStrength);
                }
            }
            break;
            
        case HAPTIC_CENTERING:
            if (elapsedTime >= _centeringDuration) {
                _hapticState = HAPTIC_FINAL_BUMP;
                _hapticStartTime = currentTime;
                
                // Final "bump" to create a definitive feel
                if (_hapticDirection > 0) {
                    motorDriver.right_motor_forward(_finalBumpStrength);
                } else {
                    motorDriver.right_motor_backward(_finalBumpStrength);
                }
            }
            break;
            
        case HAPTIC_FINAL_BUMP:
            if (elapsedTime >= _finalBumpDuration) {
                // Stop the motor completely
                motorDriver.right_motor_stop();
                
                // Enter recovery state (prevents immediate re-triggering)
                _hapticState = HAPTIC_RECOVERY;
                _hapticStartTime = currentTime;
            }
            break;
            
        case HAPTIC_RECOVERY:
            if (elapsedTime >= _recoveryDuration) {
                // Make sure motor is stopped before exiting sequence
                motorDriver.right_motor_stop();
                _hapticState = HAPTIC_IDLE;
            }
            break;
            
        default:
            _hapticState = HAPTIC_IDLE;
            motorDriver.right_motor_stop();
            break;
    }
}
