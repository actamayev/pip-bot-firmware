#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdint>
#pragma once

extern const uint8_t LED_PIN;
extern const uint8_t NUM_LEDS;

extern const uint8_t DNS_PORT;

extern const uint8_t DIGITAL_IR_PIN_1;
extern const uint8_t DIGITAL_IR_PIN_2;
extern const uint8_t DIGITAL_IR_PIN_3;

//i2c:
extern const uint8_t TIME_OF_FLIGHT_SDA;
extern const uint8_t TIME_OF_FLIGHT_SCL;

// tof:
extern const uint8_t TIME_OF_FLIGHT_XSHUT_1;
extern const uint8_t TIME_OF_FLIGHT_XSHUT_2;
extern const uint8_t TIME_OF_FLIGHT_XSHUT_3;

extern const uint8_t TOF_SENSOR1_ADDRESS;
extern const uint8_t TOF_SENSOR2_ADDRESS;
extern const uint8_t TOF_SENSOR3_ADDRESS;

//IMU:
extern const uint8_t IMU_SCL;
extern const uint8_t IMU_MISO;
extern const uint8_t IMU_MOSI;
extern const uint8_t IMU_CS;

extern const char* rootCACertificate;

enum class PathHeader {
    Auth
};

enum class PathFooter {
    Login,
    Logout
};

// Getter functions for URLs based on the environment
const char* getEnvironment();
const char* getServerUrl();
const char* getWsServerUrl();
const char* getPipID();

extern std::string getAPSSID(); 

#endif
