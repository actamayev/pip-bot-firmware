#pragma once
#include <string>
#include <cstdint>

extern const uint8_t ESP_LED_PIN1;
extern const uint8_t NUM_LEDS1;
extern const uint8_t ESP_LED_PIN2;
extern const uint8_t NUM_LEDS2;
extern const uint8_t MAX_LED_BRIGHTNESS;

//I2C:
extern const uint8_t I2C_SDA;
extern const uint8_t I2C_SCL;
extern const uint32_t I2C_CLOCK_SPEED;

// Side TOFs:
extern const uint8_t LEFT_TOF_ADDRESS;
extern const uint8_t RIGHT_TOF_ADDRESS;

// Multizone TOF
extern const uint8_t MULTIZONE_TOF_ADDRESS;
extern const uint8_t TOF_IMAGE_RESOLUTION;
extern const uint8_t TOF_RANGING_FREQUENCY;

// IMU:
extern const uint16_t IMU_UPDATE_FREQ_MICROSECS;
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

// Display Screen
extern const uint8_t SCREEN_WIDTH;
extern const uint8_t SCREEN_HEIGHT;
extern const int8_t OLED_RESET;
extern const uint8_t SCREEN_ADDRESS;

// Color Sensor
extern const uint8_t COLOR_SENSOR_LED_PIN;

//IR sensor
extern const uint8_t PIN_MUX_C;
extern const uint8_t PIN_MUX_B;
extern const uint8_t PIN_MUX_A;
extern const uint8_t PIN_MUX_OUT;
extern const uint8_t PIN_IR_EN;

//Speaker
extern const uint8_t AUDIO_PIN;

// Buttons
extern const uint8_t BUTTON_PIN_1;
extern const uint8_t BUTTON_PIN_2;

// Assign Stack sizes for the two cores
extern const uint32_t SENSOR_STACK_SIZE;
extern const uint32_t NETWORK_STACK_SIZE;

// Web/Pip/Environment
extern const char* rootCACertificate;

// Getter functions for URLs based on the environment
const char* getEnvironment();
const char* getServerFirmwareEndpoint();
const char* getWsServerUrl();
const char* getPipID();
