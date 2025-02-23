#include "./include/config.h"
#include "./include/rgb_led.h"
#include "./include/motor_driver.h"
#include "./include/lab_demo_manager.h"

void LabDemoManager::handleBinaryMessage(const char* data, size_t length) {
    if (length != 2) {
        Serial.println("Invalid binary message length");
        return;
    }

    if (data[0] != 1) {  // 1 = motor control
        Serial.printf("Unknown message type: %d\n", data[0]);
        return;
    }

    uint8_t motorByte = data[1];
    uint8_t leftValue = (motorByte >> 4) & 0x0F;  // First 4 bits
    uint8_t rightValue = motorByte & 0x0F;        // Last 4 bits

    // Map 0, 1, 2 to -255, 0, 255
    int leftSpeed = (leftValue == 0) ? -255 : (leftValue == 1) ? 0 : 255;
    int rightSpeed = (rightValue == 0) ? -255 : (rightValue == 1) ? 0 : 222;

    Serial.printf("Received motor control - Left: %d, Right: %d\n", leftSpeed, rightSpeed);
    updateMotorSpeeds(leftSpeed, rightSpeed);
}

void LabDemoManager::updateMotorSpeeds(int leftSpeed, int rightSpeed) {
    Serial.printf("Motors updated - Left: %d, Right: %d\n", leftSpeed, rightSpeed);

    // Handle left motor
    if (leftSpeed > 0) {
        motorDriver.left_motor_forward(leftSpeed);
    } else if (leftSpeed < 0) {
        motorDriver.left_motor_backward(-leftSpeed);
    } else {
        motorDriver.left_motor_stop();
    }

    // Handle right motor
    if (rightSpeed > 0) {
        motorDriver.right_motor_forward(rightSpeed);
    } else if (rightSpeed < 0) {
        motorDriver.right_motor_backward(-rightSpeed);
    } else {
        motorDriver.right_motor_stop();
    }

    // Update RGB LED based on motion
    if (leftSpeed == 0 && rightSpeed == 0) {
        rgbLed.turn_led_off();
    } else if (leftSpeed > 0 && rightSpeed > 0) {
        rgbLed.set_led_blue();  // Forward
    } else if (leftSpeed < 0 && rightSpeed < 0) {
        rgbLed.set_led_red();   // Backward
    } else {
        rgbLed.set_led_green(); // Turning or mixed
    }
}
