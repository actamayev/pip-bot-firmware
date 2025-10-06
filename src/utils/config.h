#pragma once
#include <string>
#include <Wire.h>   
#include <cstdint>

// LED Configuration
constexpr uint8_t MAX_LED_BRIGHTNESS = 75;

// I2C Configuration
constexpr uint8_t I2C_SDA_1 = 18;
constexpr uint8_t I2C_SCL_1 = 8;
constexpr uint32_t I2C_1_CLOCK_SPEED = 800 * 1000; // 800 kHz
constexpr uint8_t I2C_SDA_2 = 9; // Battery monitor, IMU
constexpr uint8_t I2C_SCL_2 = 10; // Battery monitor, IMU
constexpr uint32_t I2C_2_CLOCK_SPEED = 100 * 1000; // 100 kHz (works best for IMU)

// Motors + Encoders
constexpr uint8_t LEFT_MOTOR_PIN_IN_1 = 39;
constexpr uint8_t LEFT_MOTOR_PIN_IN_2 = 40;
constexpr uint8_t LEFT_MOTOR_ENCODER_A = 48;
constexpr uint8_t LEFT_MOTOR_ENCODER_B = 47;

constexpr uint8_t RIGHT_MOTOR_PIN_IN_1 = 42;
constexpr uint8_t RIGHT_MOTOR_PIN_IN_2 = 41;
constexpr uint8_t RIGHT_MOTOR_ENCODER_A = 2;
constexpr uint8_t RIGHT_MOTOR_ENCODER_B = 1;

constexpr uint8_t MAX_MOTOR_SPEED = 255;

// Display Screen
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr int8_t OLED_RESET = -1;  // Reset pin (-1 if sharing Arduino reset pin)
constexpr uint8_t SCREEN_ADDRESS = 0x3C;

// Buttons
constexpr uint8_t LEFT_BUTTON_PIN = 11; // Left
constexpr uint8_t RIGHT_BUTTON_PIN = 12; // Right

// Assign Stack sizes for the two cores
constexpr uint16_t MAX_PROGRAM_SIZE = 8192;
constexpr uint16_t PWR_EN = 38;

// curl -s https://www.amazontrust.com/repository/AmazonRootCA1.pem
constexpr const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n"
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
"rqXRfboQnoZsG4q5WTP468SQvvG5\n"
"-----END CERTIFICATE-----\n";

inline const char* getEnvironment() {
    return DEFAULT_ENVIRONMENT;
}

inline const char* getServerFirmwareEndpoint() {
    std::string env = getEnvironment();
    if (env == "local") {
        return "http://10.176.185.220:8080/pip/firmware-update";
    } else if (env == "staging") {
        return "https://staging-api.leverlabs.com/pip/firmware-update";
    }
    return "https://production-api.leverlabs.com/pip/firmware-update";
}

inline const char* getWsServerUrl() {
    std::string env = getEnvironment();
    if (env == "local") {
        return "ws://10.176.185.220:8080/esp32";
    } else if (env == "staging") {
        return "wss://staging-api.leverlabs.com/esp32";
    }
    return "wss://production-api.leverlabs.com/esp32";
}
