#include "../utils/config.h"
#include "./lab_demo_manager.h"
#include "../actuators/rgb_led.h"
#include "../actuators/motor_driver.h"
#include "../sensors/encoder_manager.h"
#include "../actuators/speaker.h"

LabDemoManager::LabDemoManager() 
    : isExecutingCommand(false), 
      hasNextCommand(false),
      currentLeftSpeed(0),
      currentRightSpeed(0),
      nextLeftSpeed(0),
      nextRightSpeed(0),
      startLeftCount(0),
      startRightCount(0) {
}

void LabDemoManager::handleMotorControl(const uint8_t* data) {
    // Extract 16-bit signed integers (little-endian)
    int16_t leftSpeed = static_cast<int16_t>(data[1] | (data[2] << 8));
    int16_t rightSpeed = static_cast<int16_t>(data[3] | (data[4] << 8));
    
    updateMotorSpeeds(leftSpeed, rightSpeed);
}

// Add this to lab_demo_manager.cpp
void LabDemoManager::handleSoundCommand(SoundType soundType) {
    // Play the requested tune
    switch(soundType) {
        case SoundType::ALERT:
            Serial.println("Playing Alert sound");
            // Call your alert sound function
            // speaker.alert();
            break;

        case SoundType::BEEP:
            Serial.println("Playing Beep sound");
            // Call your beep sound function
            // speaker.beep();
            break;

        case SoundType::CHIME:
            Serial.println("Playing Chime sound");
            speaker.chime();
            break;

        default:
            Serial.printf("Unknown tune type: %d\n", static_cast<int>(soundType));
            break;
    }
}

void LabDemoManager::handleSpeakerMute(SpeakerStatus status) {
    if (status == SpeakerStatus::MUTED) {
        Serial.println("Muting speaker");
        speaker.mute();
    } else if (status == SpeakerStatus::UNMUTED) {
        Serial.println("Unmuting speaker");
        speaker.unmute();
    } else {
        Serial.printf("Unknown mute state: %d\n", static_cast<int>(status));
    }
}

void LabDemoManager::updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed) {
    // Constrain speeds
    leftSpeed = constrain(leftSpeed, -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);
    
    // If we're not executing a command, start this one immediately
    if (!isExecutingCommand) {
        executeCommand(leftSpeed, rightSpeed);
    } else {
        // Store as next command
        hasNextCommand = true;
        nextLeftSpeed = leftSpeed;
        nextRightSpeed = rightSpeed;
    }
}

void LabDemoManager::executeCommand(int16_t leftSpeed, int16_t rightSpeed) {
    // Save command details
    currentLeftSpeed = leftSpeed;
    currentRightSpeed = rightSpeed;

    // Get initial encoder counts directly
    startLeftCount = encoderManager._leftEncoder.getCount();
    startRightCount = encoderManager._rightEncoder.getCount();

    Serial.printf("Motors updated - Left: %d, Right: %d\n", leftSpeed, rightSpeed);

    motorDriver.set_motor_speeds(leftSpeed, rightSpeed);

    // Enable straight driving correction for forward/backward movement
    if ((leftSpeed > 0 && rightSpeed > 0) || (leftSpeed < 0 && rightSpeed < 0)) {
        motorDriver.enable_straight_driving();
    } else {
        motorDriver.disable_straight_driving();
    }

    // Update LED based on motor direction (unchanged)
    if (leftSpeed == 0 && rightSpeed == 0) {
        rgbLed.turn_led_off();
    } else if (leftSpeed > 0 && rightSpeed > 0) {
        rgbLed.set_led_blue();
    } else if (leftSpeed < 0 && rightSpeed < 0) {
        rgbLed.set_led_red();
    } else {
        rgbLed.set_led_green();
    }
    
    isExecutingCommand = true;
}

void LabDemoManager::processPendingCommands() {
    if (_balancingEnabled == BalanceStatus::BALANCED) {
        return updateBalancing();
    }

    motorDriver.update_motor_speeds();
    if (!isExecutingCommand) {
        // If we have a next command, execute it
        if (hasNextCommand) {
            executeCommand(nextLeftSpeed, nextRightSpeed);
            hasNextCommand = false;
        }
        return;
    }

    // Is this a movement command (non-zero speed)?
    bool isMovementCommand = (currentLeftSpeed != 0 || currentRightSpeed != 0);

    // For stop commands, mark as completed immediately and execute next command
    if (!isMovementCommand) {
        isExecutingCommand = false;
        
        if (hasNextCommand) {
            executeCommand(nextLeftSpeed, nextRightSpeed);
            hasNextCommand = false;
        }
        return;
    }

    // Get current encoder counts
    int64_t currentLeftCount = encoderManager._leftEncoder.getCount();
    int64_t currentRightCount = encoderManager._rightEncoder.getCount();
    
    // Calculate absolute change in encoder counts
    int64_t leftDelta = abs(currentLeftCount - startLeftCount);
    int64_t rightDelta = abs(currentRightCount - startRightCount);
    
    // Optional debugging (only printed once every 500ms)
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 500) {
        Serial.printf("Encoder deltas - Left: %lld, Right: %lld (Target: %d)\n", 
                     leftDelta, rightDelta, MIN_ENCODER_PULSES);
        lastDebugTime = millis();
    }
    
    // Command is complete if either encoder has moved enough
    if (leftDelta >= MIN_ENCODER_PULSES || rightDelta >= MIN_ENCODER_PULSES) {
        Serial.printf("Command completed with pulses - Left: %lld, Right: %lld\n", 
                     leftDelta, rightDelta);
        
        isExecutingCommand = false;
        
        if (hasNextCommand) {
            executeCommand(nextLeftSpeed, nextRightSpeed);
            hasNextCommand = false;
        }
    }
}

void LabDemoManager::handleBalanceCommand(BalanceStatus status) {
    if (status == BalanceStatus::BALANCED && _balancingEnabled == BalanceStatus::UNBALANCED) {
        // Starting balance mode
        _balancingEnabled = BalanceStatus::BALANCED;

        // Reset PID variables
        _errorSum = 0.0f;
        _lastError = 0.0f;
        _lastBalanceUpdateTime = millis();

        float currentAngle = Sensors::getInstance().getPitch();
        _lastValidAngle = currentAngle;

        // Initialize both buffers with the current angle
        for (uint8_t i = 0; i < ANGLE_BUFFER_SIZE; i++) {
            _controlBuffer[i] = currentAngle;
            _safetyBuffer[i] = currentAngle;
        }
        _controlBufferIndex = 0;
        _controlBufferCount = ANGLE_BUFFER_SIZE;
        _safetyBufferIndex = 0;
        _safetyBufferCount = ANGLE_BUFFER_SIZE;

        // Disable straight driving correction as we're taking control
        motorDriver.disable_straight_driving();

        // Set LED to indicate balancing mode
        rgbLed.set_led_purple(); // Assuming you have this color method
        
        Serial.println("Balance mode enabled");
    } else if (status == BalanceStatus::UNBALANCED && _balancingEnabled == BalanceStatus::BALANCED) {
        // Stopping balance mode
        _balancingEnabled = BalanceStatus::UNBALANCED;
        
        // Stop motors immediately
        motorDriver.stop_both_motors();
        
        // Indicate mode change
        rgbLed.turn_led_off();
        
        Serial.println("Balance mode disabled");
    }
}

void LabDemoManager::updateBalancing() {
    if (_balancingEnabled != BalanceStatus::BALANCED) return;

    unsigned long currentTime = millis();
    if (currentTime - _lastBalanceUpdateTime < BALANCE_UPDATE_INTERVAL) {
        return; // Maintain update rate at 100Hz
    }
    _lastBalanceUpdateTime = currentTime;

    // Get current roll (which is actually pitch in your system)
    float rawAngle = Sensors::getInstance().getPitch();
    float currentAngle = rawAngle; // Will be either raw or last valid reading

    Serial.printf("rawAngle %f\n", rawAngle);

    // 1. Update safety monitoring buffer (always accepts all readings)
    _safetyBuffer[_safetyBufferIndex] = rawAngle;
    _safetyBufferIndex = (_safetyBufferIndex + 1) % ANGLE_BUFFER_SIZE;
    if (_safetyBufferCount < ANGLE_BUFFER_SIZE) _safetyBufferCount++;
    
    // Calculate safety buffer average
    float safetyAverage = 0.0f;
    if (_safetyBufferCount > 0) {
        float sum = 0.0f;
        for (uint8_t i = 0; i < _safetyBufferCount; i++) {
            sum += _safetyBuffer[i];
        }
        safetyAverage = sum / _safetyBufferCount;
    }
    
    // 2. Calculate average of control buffer (for outlier detection)
    float controlAverage = 0.0f;
    if (_controlBufferCount > 0) {
        float sum = 0.0f;
        for (uint8_t i = 0; i < _controlBufferCount; i++) {
            sum += _controlBuffer[i];
        }
        controlAverage = sum / _controlBufferCount;
    }
    
    // 3. Validate reading against control average for PID input
    float deviation = abs(rawAngle - controlAverage);
    if (deviation > MAX_SAFE_ANGLE_DEVIATION / 3 && _controlBufferCount > 0) {
        // Reading differs too much from average - use last valid reading
        currentAngle = _lastValidAngle;
        Serial.printf("Rejecting outlier: %.2f (avg: %.2f, dev: %.2f)\n", 
                    rawAngle, controlAverage, deviation);
    } else {
        // Reading is valid - update last valid angle and control buffer
        _lastValidAngle = rawAngle;
        
        // Add to control buffer
        _controlBuffer[_controlBufferIndex] = rawAngle;
        _controlBufferIndex = (_controlBufferIndex + 1) % ANGLE_BUFFER_SIZE;
        if (_controlBufferCount < ANGLE_BUFFER_SIZE) _controlBufferCount++;
    }

    // 4. Safety check using unfiltered safety buffer
    if (abs(safetyAverage - _targetAngle) > MAX_SAFE_ANGLE_DEVIATION) {
        Serial.printf("Safety cutoff triggered: Avg Angle %.2f exceeds limits\n", safetyAverage);
        handleBalanceCommand(BalanceStatus::UNBALANCED); // Turn off balancing
        return;
    } else {
        rgbLed.set_led_green();
    }
    
    // 5. PID calculation using filtered angle for control
    float error = _targetAngle - currentAngle;
    float deltaTime = BALANCE_UPDATE_INTERVAL / 1000.0f; // Convert to seconds
    
    // Update integral term with anti-windup
    _errorSum += error * deltaTime;
    _errorSum = constrain(_errorSum, -10.0f, 10.0f); // Prevent excessive buildup
    
    // If error crosses zero, reduce integral term to prevent oscillation
    if ((error > 0 && _lastError < 0) || (error < 0 && _lastError > 0)) {
        _errorSum *= 0.8f; // Reduce by 20% when crossing zero
    }
    
    // Get gyro rate directly
    float gyroRate = Sensors::getInstance().getYRotationRate(); 
    
    // Calculate motor power using PID formula
    float proportionalTerm = BALANCE_P_GAIN * error;
    float integralTerm = BALANCE_I_GAIN * _errorSum;
    float derivativeTerm = BALANCE_D_GAIN * -gyroRate; // Negative because we want to counter rotation
    
    int16_t motorPower = constrain(
        (int16_t)(proportionalTerm + integralTerm + derivativeTerm),
        -MAX_BALANCE_POWER, 
        MAX_BALANCE_POWER
    );
    
    // Apply motor power
    motorDriver.set_motor_speeds(motorPower, motorPower);
    motorDriver.update_motor_speeds(); // Force immediate update
    
    // Store this error for next iteration
    _lastError = error;
    
    // Debug output (limit frequency to avoid overloading Serial)
    static unsigned long lastDebugTime = 0;
    if (currentTime - lastDebugTime > 100) { // Print every 100ms
        Serial.printf("Raw: %.2f, Control: %.2f, Safety: %.2f, Error: %.2f, P: %.2f, I: %.2f, D: %.2f, Power: %d\n",
                     rawAngle, currentAngle, safetyAverage, error, 
                     proportionalTerm, integralTerm, derivativeTerm, motorPower);
        lastDebugTime = currentTime;
    }
}
