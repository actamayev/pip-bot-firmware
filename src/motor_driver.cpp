#include "./include/config.h"
#include "./include/motor_driver.h"

MotorDriver motorDriver;

void MotorDriver::both_motors_forward() {
    left_motor_forward();
    right_motor_forward();
}

void MotorDriver::both_motors_backward() {
    left_motor_backward();
    right_motor_backward();
}

void MotorDriver::stop_both_motors() {
    left_motor_stop();
    right_motor_stop();
}

void MotorDriver::rotate_clockwise() {
    left_motor_forward();
    right_motor_backward();
}

void MotorDriver::rotate_counterclockwise() {
    left_motor_backward();
    right_motor_forward();
}

void MotorDriver::left_motor_forward() {
    digitalWrite(LEFT_MOTOR_PIN_IN_1, LOW);
    analogWrite(LEFT_MOTOR_PIN_IN_2, 255);  // Full speed forward  
}

void MotorDriver::left_motor_backward() {
    digitalWrite(LEFT_MOTOR_PIN_IN_2, LOW);
    analogWrite(LEFT_MOTOR_PIN_IN_1, 255);  // Full speed backward
}

void MotorDriver::left_motor_stop() {
    analogWrite(LEFT_MOTOR_PIN_IN_1, 0);
    analogWrite(LEFT_MOTOR_PIN_IN_2, 0);
}

void MotorDriver::right_motor_forward() {
    digitalWrite(RIGHT_MOTOR_PIN_IN_1, LOW);
    analogWrite(RIGHT_MOTOR_PIN_IN_2, 222);  // Full speed forward  
}

void MotorDriver::right_motor_backward() {
    digitalWrite(RIGHT_MOTOR_PIN_IN_2, LOW);
    analogWrite(RIGHT_MOTOR_PIN_IN_1, 255);  // Full speed backward
}

void MotorDriver::right_motor_stop() {
    analogWrite(RIGHT_MOTOR_PIN_IN_1, 0);
    analogWrite(RIGHT_MOTOR_PIN_IN_2, 0);
}
