#pragma once
#include <string>
#include <cstdint>

extern const uint8_t ESP_LED_PIN;
extern const uint8_t NUM_LEDS;

extern const uint8_t DNS_PORT;

//I2C:
extern const uint8_t I2C_SDA;
extern const uint8_t I2C_SCL;
extern const uint32_t I2C_CLOCK_SPEED;

// TOF:
extern const uint8_t LEFT_TOF_RESET_PIN;
extern const uint8_t RIGHT_TOF_RESET_PIN;

// Unique I2C addresses for each sensor
extern const uint8_t DEFAULT_TOF_I2C_ADDRESS;
extern const uint8_t LEFT_TOF_ADDRESS;
extern const uint8_t RIGHT_TOF_ADDRESS;

extern const uint8_t TOF_IMAGE_RESOLUTION;
extern const uint8_t TOF_RANGING_FREQUENCY;

// IMU:
extern const uint32_t IMU_UPDATE_FREQ_MICROSECS;
extern const uint8_t IMU_DEFAULT_ADDRESS;

// Motors + Encoders
extern const uint8_t LEFT_MOTOR_PIN_IN_1;
extern const uint8_t LEFT_MOTOR_PIN_IN_2;
extern const uint8_t LEFT_MOTOR_ENCODER_A;
extern const uint8_t LEFT_MOTOR_ENCODER_B;

extern const uint8_t RIGHT_MOTOR_PIN_IN_1;
extern const uint8_t RIGHT_MOTOR_PIN_IN_2;
extern const uint8_t RIGHT_MOTOR_ENCODER_A;
extern const uint8_t RIGHT_MOTOR_ENCODER_B;

//IR sensor
extern const int PIN_MUX_C;
extern const int PIN_MUX_B;
extern const int PIN_MUX_A;
extern const int PIN_MUX_OUT;
extern const int PIN_IR_EN;

// Assign Stack sizes for the two cores
extern const uint32_t SENSOR_STACK_SIZE;
extern const uint32_t NETWORK_STACK_SIZE;

// Web/Pip/Environment
extern const char* rootCACertificate;

// Getter functions for URLs based on the environment
const char* getEnvironment();
const char* getServerUrl();
const char* getWsServerUrl();
const char* getPipID();

extern std::string getAPSSID(); 
