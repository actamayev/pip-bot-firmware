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
