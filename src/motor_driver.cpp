#include "./include/config.h"
#include "./include/motor_driver.h"

MotorDriver motorDriver;

MotorDriver::MotorDriver() {
    // Initialize motor pins
    pinMode(LEFT_MOTOR_PIN_IN_1, OUTPUT);
    pinMode(LEFT_MOTOR_PIN_IN_2, OUTPUT);
    pinMode(RIGHT_MOTOR_PIN_IN_1, OUTPUT);
    pinMode(RIGHT_MOTOR_PIN_IN_2, OUTPUT);

    // Initialize encoder pins
    pinMode(LEFT_MOTOR_ENCODER_A, INPUT_PULLUP);
    pinMode(LEFT_MOTOR_ENCODER_B, INPUT_PULLUP);
    pinMode(RIGHT_MOTOR_ENCODER_A, INPUT_PULLUP);
    pinMode(RIGHT_MOTOR_ENCODER_B, INPUT_PULLUP);

    // Attach interrupts for encoders (assuming ENC1_A and ENC2_A are defined in config.h)
    // attachInterrupt(LEFT_MOTOR_ENCODER_A, encoder1_isr, RISING);
    // attachInterrupt(RIGHT_MOTOR_ENCODER_A, encoder2_isr, RISING);
}

void MotorDriver::both_motors_forward(uint8_t speed) {
    left_motor_forward(speed);
    right_motor_forward(speed);
}

void MotorDriver::both_motors_backward(uint8_t speed) {
    left_motor_backward(speed);
    right_motor_backward(speed);
}

void MotorDriver::stop_both_motors() {
    left_motor_stop();
    right_motor_stop();
}

void MotorDriver::rotate_clockwise(uint8_t speed) {
    left_motor_forward(speed);
    right_motor_backward(speed);
}

void MotorDriver::rotate_counterclockwise(uint8_t speed) {
    left_motor_backward(speed);
    right_motor_forward(speed);
}

void MotorDriver::left_motor_forward(uint8_t speed) {
    analogWrite(LEFT_MOTOR_PIN_IN_1, 0); // Explicitly clear backward pin
    digitalWrite(LEFT_MOTOR_PIN_IN_1, LOW);
    analogWrite(LEFT_MOTOR_PIN_IN_2, speed);
}

void MotorDriver::left_motor_backward(uint8_t speed) {
    analogWrite(LEFT_MOTOR_PIN_IN_2, 0); // Explicitly clear forward pin
    digitalWrite(LEFT_MOTOR_PIN_IN_2, LOW);
    analogWrite(LEFT_MOTOR_PIN_IN_1, speed);
}

void MotorDriver::left_motor_stop() {
    analogWrite(LEFT_MOTOR_PIN_IN_1, 0);
    analogWrite(LEFT_MOTOR_PIN_IN_2, 0);
}

void MotorDriver::right_motor_forward(uint8_t speed) {
    analogWrite(RIGHT_MOTOR_PIN_IN_1, 0); // Explicitly clear backward pin
    digitalWrite(RIGHT_MOTOR_PIN_IN_1, LOW);
    analogWrite(RIGHT_MOTOR_PIN_IN_2, speed);
}

void MotorDriver::right_motor_backward(uint8_t speed) {
    analogWrite(RIGHT_MOTOR_PIN_IN_2, 0); // Explicitly clear forward pin
    digitalWrite(RIGHT_MOTOR_PIN_IN_2, LOW);
    analogWrite(RIGHT_MOTOR_PIN_IN_1, speed);
}

void MotorDriver::right_motor_stop() {
    analogWrite(RIGHT_MOTOR_PIN_IN_1, 0);
    analogWrite(RIGHT_MOTOR_PIN_IN_2, 0);
}

// void IRAM_ATTR MotorDriver::encoder1_isr() {
//     motorDriver.leftEncoderCount++;  // Increment left encoder count
// }

// void IRAM_ATTR MotorDriver::encoder2_isr() {
//     motorDriver.rightEncoderCount++;  // Increment right encoder count
// }

// void MotorDriver::setTargetDistance(int16_t leftSpeed, int16_t rightSpeed) {
//     leftSpeed = constrain(leftSpeed, -255, 255);
//     rightSpeed = constrain(rightSpeed, -255, 255);

//     if (leftSpeed == 0 && rightSpeed == 0) {
//         leftTargetCount = leftEncoderCount;  // Stop by setting target to current position
//         rightTargetCount = rightEncoderCount;
//         leftCurrentSpeed = 0;
//         rightCurrentSpeed = 0;
//     } else {
//         // For each command, add/subtract TICKS_PER_COMMAND based on direction
//         leftCurrentSpeed = abs(leftSpeed);
//         rightCurrentSpeed = abs(rightSpeed);
//         int leftDirection = (leftSpeed > 0) ? 1 : (leftSpeed < 0) ? -1 : 0;
//         int rightDirection = (rightSpeed > 0) ? 1 : (rightSpeed < 0) ? -1 : 0;
//         leftTargetCount += leftDirection * TICKS_PER_COMMAND;
//         rightTargetCount += rightDirection * TICKS_PER_COMMAND;
//     }
// }

// void MotorDriver::updateMotorControl() {
//     // Left motor control
//     if (leftEncoderCount < leftTargetCount) {
//         left_motor_forward(leftCurrentSpeed);
//     } else if (leftEncoderCount > leftTargetCount) {
//         left_motor_backward(leftCurrentSpeed);
//     } else {
//         left_motor_stop();
//     }

//     // Right motor control
//     if (rightEncoderCount < rightTargetCount) {
//         right_motor_forward(rightCurrentSpeed);
//     } else if (rightEncoderCount > rightTargetCount) {
//         right_motor_backward(rightCurrentSpeed);
//     } else {
//         right_motor_stop();
//     }
// }
