#include "./include/config.h"
#include "./include/rgb_led.h"
#include "./include/motor_driver.h"
#include "./include/lab_demo_manager.h"
#include "./include/encoder_manager.h"

void LabDemoManager::handleBinaryMessage(const char* data) {
    if (data[0] != 1) {  // 1 = motor control
        Serial.printf("Unknown message type: %d\n", data[0]);
        return;
    }

    // Extract 16-bit signed integers (little-endian)
    int16_t leftSpeed = (static_cast<uint8_t>(data[2]) << 8) | static_cast<uint8_t>(data[1]);
    int16_t rightSpeed = (static_cast<uint8_t>(data[4]) << 8) | static_cast<uint8_t>(data[3]);

    updateMotorSpeeds(leftSpeed, rightSpeed);
}

void LabDemoManager::updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed) {
    leftSpeed = constrain(leftSpeed, -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);

    Serial.printf("Motors updated - Left: %d, Right: %d\n", leftSpeed, rightSpeed);

    if (leftSpeed == 0) {
        motorDriver.left_motor_stop();
    } else if (leftSpeed > 0) {
        motorDriver.left_motor_forward(leftSpeed);
    } else {
        motorDriver.left_motor_backward(-leftSpeed);
    }

    if (rightSpeed == 0) {
        motorDriver.right_motor_stop();
    } else if (rightSpeed > 0) {
        motorDriver.right_motor_forward(rightSpeed);
    } else {
        motorDriver.right_motor_backward(-rightSpeed);
    }

    encoderManager.getInstance().should_log_motor_rpm = true;

    if (leftSpeed == 0 && rightSpeed == 0) {
        rgbLed.turn_led_off();
    } else if (leftSpeed > 0 && rightSpeed > 0) {
        rgbLed.set_led_blue();
    } else if (leftSpeed < 0 && rightSpeed < 0) {
        rgbLed.set_led_red();
    } else {
        rgbLed.set_led_green();
    }
}

void LabDemoManager::monitorEncoders() {
    // Check if motors are active
    WheelRPMs wheelRpms = encoderManager.getInstance().getBothWheelRPMs();

    Serial.printf("left wheel RPM%d\n", wheelRpms.leftWheelRPM);
    Serial.printf("right wheel RPM%d\n", wheelRpms.rightWheelRPM);

    // if (leftWheelRpm == 0 && rightWheelRpm == 0) return;
    // // Check if it's time to send encoder data
    // unsigned long currentTime = millis();
    // if (currentTime - _lastEncoderUpdateTime >= ENCODER_UPDATE_INTERVAL) {
    //     Serial.println("Updating encoder readings...");

    //     // Update encoder calculations
    //     encoderManager.update();

    //     // Get current wheel speeds
    //     float leftWheelRPM = encoderManager.getLeftWheelRPM();
    //     float rightWheelRPM = encoderManager.getRightWheelRPM();
        
    //     // Send encoder data to server
    //     sendEncoderDataToServer(leftWheelRPM, rightWheelRPM);
        
    //     // Debug output to serial
    //     Serial.printf("Encoder Data - Left RPM: %.2f, Right RPM: %.2f\n", 
    //                     leftWheelRPM, rightWheelRPM);

    //     // Update last update time
    //     _lastEncoderUpdateTime = currentTime;
    // }
}

// void LabDemoManager::sendEncoderDataToServer(float leftWheelRPM, float rightWheelRPM) {
//     Serial.println("Would send encoder data to server here");
//     // Add your socket logic here to send data back to server
//     // Example data format might be:
//     // {
//     //   "leftRPM": leftWheelRPM,
//     //   "rightRPM": rightWheelRPM,
//     //   "leftSpeed": _leftMotorSpeed,
//     //   "rightSpeed": _rightMotorSpeed
//     // }
    
//     // placeholder for your socket sending logic
// }
