#include "./include/config.h"
#include "./include/rgb_led.h"
#include "./include/motor_driver.h"
#include "./include/lab_demo_manager.h"

void LabDemoManager::handleBinaryMessage(const char* data, size_t length) {
    if (length != 2) {
        Serial.println("Invalid binary message length");
        return;
    }

    // Check message type
    uint8_t messageType = data[0];
    if (messageType != 1) {  // 1 = motor control
        Serial.printf("Unknown message type: %d\n", messageType);
        return;
    }

    // Extract motor values from second byte
    uint8_t motorByte = data[1];
    uint8_t leftValue = (motorByte >> 4) & 0x0F;   // Get first 4 bits
    uint8_t rightValue = motorByte & 0x0F;         // Get last 4 bits

    // Convert to motor speeds
    int leftSpeed = byteToMotorSpeed(leftValue);
    int rightSpeed = byteToMotorSpeed(rightValue);

    Serial.printf("Received motor control - Left: %d, Right: %d\n", leftSpeed, rightSpeed);
    updateMotorSpeeds(leftSpeed, rightSpeed);
}

void LabDemoManager::updateMotorSpeeds(int leftMotor, int rightMotor) {
    Serial.printf("Motors updated - Left: %d, Right: %d\n", leftMotor, rightMotor);

    if (leftMotor == -1 && rightMotor == -1) {
        rgbLed.set_led_red();
        motorDriver.both_motors_backward();
        return;
    } else if (leftMotor == -1 && rightMotor == 1) {
        rgbLed.set_led_green();
        motorDriver.rotate_counterclockwise();
        return;
    } else if (leftMotor == 1 && rightMotor == -1) {
        rgbLed.set_led_white();
        motorDriver.rotate_clockwise();
        return;
    } else if (leftMotor == 1 && rightMotor == 1) {
        rgbLed.set_led_blue();
        motorDriver.both_motors_forward();
        return;
    } else {
        rgbLed.turn_led_off();
        motorDriver.stop_both_motors();
        return;
    }
}
