#include "./config.h"

//Blank to be consistent with config.h

const uint8_t ESP_LED_PIN1 = 38;
const uint8_t NUM_LEDS1 = 2;
const uint8_t ESP_LED_PIN2 = 4;
const uint8_t NUM_LEDS2 = 4;
const uint8_t MAX_LED_BRIGHTNESS = 150;

//I2C
const uint8_t I2C_SDA = 18;
const uint8_t I2C_SCL = 8;
const uint32_t I2C_CLOCK_SPEED = 400 * 1000; // 400 kHz

// Side TOFs:
const uint8_t LEFT_TOF_ADDRESS = 0x60;
const uint8_t RIGHT_TOF_ADDRESS = 0x51;

// Multizone TOF
const uint8_t MULTIZONE_TOF_ADDRESS = 0x29; // Default address
const uint8_t TOF_IMAGE_RESOLUTION = 8;  // Image width (can be 4 or 8)
const uint8_t TOF_RANGING_FREQUENCY = 15;  // TOF sampling frequency

// IMU
const uint16_t IMU_UPDATE_FREQ_MICROSECS = 5000;  // 5ms, 200Hz
const uint8_t IMU_DEFAULT_ADDRESS = 0x4A;

// Motors + Encoders
const uint8_t LEFT_MOTOR_PIN_IN_1 = 13;
const uint8_t LEFT_MOTOR_PIN_IN_2 = 14;
const uint8_t LEFT_MOTOR_ENCODER_A = 11;
const uint8_t LEFT_MOTOR_ENCODER_B = 10;

const uint8_t RIGHT_MOTOR_PIN_IN_1 = 21;
const uint8_t RIGHT_MOTOR_PIN_IN_2 = 47;
const uint8_t RIGHT_MOTOR_ENCODER_A = 1;
const uint8_t RIGHT_MOTOR_ENCODER_B = 2;

// Display Screen
const uint8_t SCREEN_WIDTH = 128;
const uint8_t SCREEN_HEIGHT = 64;
const int8_t OLED_RESET = -1;  // Reset pin (-1 if sharing Arduino reset pin)
const uint8_t SCREEN_ADDRESS = 0x3C;

// Color Sensor
const uint8_t COLOR_SENSOR_LED_PIN = 16;

// IR sensor
const uint8_t PIN_MUX_C = 4;    // Multiplexer C input
const uint8_t PIN_MUX_B = 5;    // Multiplexer B input
const uint8_t PIN_MUX_A = 6;    // Multiplexer A input
const uint8_t PIN_MUX_OUT = 7;  // Multiplexer output
const uint8_t PIN_IR_EN = 15;    // IR sensor enable pin

//Speaker
const uint8_t AUDIO_PIN = 9;

// Buttons
const uint8_t BUTTON_PIN_1 = 12;
const uint8_t BUTTON_PIN_2 = 48;

// Assign Stack sizes for the two cores
const uint32_t SENSOR_STACK_SIZE = 16384;  // 16KB for sensor processing
const uint32_t NETWORK_STACK_SIZE = 8192;  // 8KB for network operations

// echo | openssl s_client -showcerts -connect staging-api.bluedotrobots.com:443
const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n"
"MIIEkjCCA3qgAwIBAgITBn+USionzfP6wq4rAfkI7rnExjANBgkqhkiG9w0BAQsF\n"
"ADCBmDELMAkGA1UEBhMCVVMxEDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNj\n"
"b3R0c2RhbGUxJTAjBgNVBAoTHFN0YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4x\n"
"OzA5BgNVBAMTMlN0YXJmaWVsZCBTZXJ2aWNlcyBSb290IENlcnRpZmljYXRlIEF1\n"
"dGhvcml0eSAtIEcyMB4XDTE1MDUyNTEyMDAwMFoXDTM3MTIzMTAxMDAwMFowOTEL\n"
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
"jgSubJrIqg0CAwEAAaOCATEwggEtMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/\n"
"BAQDAgGGMB0GA1UdDgQWBBSEGMyFNOy8DJSULghZnMeyEE4KCDAfBgNVHSMEGDAW\n"
"gBScXwDfqgHXMCs4iKK4bUqc8hGRgzB4BggrBgEFBQcBAQRsMGowLgYIKwYBBQUH\n"
"MAGGImh0dHA6Ly9vY3NwLnJvb3RnMi5hbWF6b250cnVzdC5jb20wOAYIKwYBBQUH\n"
"MAKGLGh0dHA6Ly9jcnQucm9vdGcyLmFtYXpvbnRydXN0LmNvbS9yb290ZzIuY2Vy\n"
"MD0GA1UdHwQ2MDQwMqAwoC6GLGh0dHA6Ly9jcmwucm9vdGcyLmFtYXpvbnRydXN0\n"
"LmNvbS9yb290ZzIuY3JsMBEGA1UdIAQKMAgwBgYEVR0gADANBgkqhkiG9w0BAQsF\n"
"AAOCAQEAYjdCXLwQtT6LLOkMm2xF4gcAevnFWAu5CIw+7bMlPLVvUOTNNWqnkzSW\n"
"MiGpSESrnO09tKpzbeR/FoCJbM8oAxiDR3mjEH4wW6w7sGDgd9QIpuEdfF7Au/ma\n"
"eyKdpwAJfqxGF4PcnCZXmTA5YpaP7dreqsXMGz7KQ2hsVxa81Q4gLv7/wmpdLqBK\n"
"bRRYh5TmOTFffHPLkIhqhBGWJ6bt2YFGpn6jcgAKUj6DiAdjd4lpFw85hdKrCEVN\n"
"0FE6/V1dN2RMfjCyVSRCnTawXZwXgWHxyvkQAiSr6w10kY17RSlQOYiypok1JR4U\n"
"akcjMS9cmvqtmg5iUaQqqcT5NJ0hGA==\n"
"-----END CERTIFICATE-----\n";

const char* getEnvironment() {
    return DEFAULT_ENVIRONMENT;  // This is set at compile time
}

const char* getServerFirmwareEndpoint() {
    const char* env = getEnvironment();
    if (env == nullptr || std::string(env) == "local") {
        return "http://10.19.139.40:8080/pip/firmware-update";  // local can remain HTTP
    } else if (std::string(env) == "staging") {
        return "https://staging-api.bluedotrobots.com/pip/firmware-update";
    }
    return "https://production-api.bluedotrobots.com/pip/firmware-update";
}

const char* getWsServerUrl() {
    const char* env = getEnvironment();
    if (env == nullptr || std::string(env) == "local") {
        return "ws://10.19.139.40:8080/esp32";  // local default
    } else if (std::string(env) == "staging") {
        return "wss://staging-api.bluedotrobots.com/esp32";  // staging default
    }
    return "wss://production-api.bluedotrobots.com/esp32";  // production default
}

const char* getPipID() {
    return DEFAULT_PIP_ID;
}
