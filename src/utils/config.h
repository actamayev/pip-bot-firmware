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

// Side TOFs:
extern int VCNL36828P_SlaveAddress;
extern int I2C_Bus;
extern int CalibValue;
extern int AverageCount;
extern const int LEFT_TOF_ADDRESS;
extern const int RIGHT_TOF_ADDRESS;

// Multizone TOF
extern int MULTIZONE_TOF_ADDRESS;
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

// Display Screen
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;
extern const int OLED_RESET;
extern const int SCREEN_ADDRESS;

// Color Sensor
extern const uint8_t COLOR_SENSOR_LED_PIN;

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
