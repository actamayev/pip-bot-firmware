#include <string>
#include "config.h"

const uint8_t LED_PIN = 2;
const uint8_t DNS_PORT = 53;

const uint8_t DIGITAL_IR_PIN_1 = 32;
const uint8_t DIGITAL_IR_PIN_2 = 33;
const uint8_t DIGITAL_IR_PIN_3 = 34;

//i2c (same for all i2c connections):
const uint8_t TIME_OF_FLIGHT_SDA = 21;
const uint8_t TIME_OF_FLIGHT_SCL = 22;

const uint8_t TIME_OF_FLIGHT_XSHUT_1 = 4;
const uint8_t TIME_OF_FLIGHT_XSHUT_2 = 16;
const uint8_t TIME_OF_FLIGHT_XSHUT_3 = 17;

// Define unique I2C addresses for each sensor
const uint8_t TOF_SENSOR1_ADDRESS = 0x29;  // Default address
const uint8_t TOF_SENSOR2_ADDRESS = 0x30;  // New address for sensor 2
const uint8_t TOF_SENSOR3_ADDRESS = 0x31;  // New address for sensor 3

// IMU:
const uint8_t IMU_SCL = 12;
const uint8_t IMU_MISO = 14;
const uint8_t IMU_MOSI = 27;
const uint8_t IMU_CS = 26;

//Pip Information:
const char* pip_id = "DIXiC"; // Local
// const char* pip_id = "PtFX5"; // Staging
// const char* pip_id = "peiLu"; // Prod

const char* hardware_version = "0.0.1";
std::string pip_uuid = std::string(pip_id) + "-" + std::string(hardware_version);

// Pip Access point
std::string ap_ssid = "pip-" + pip_uuid;
const char* ap_password = "bluedotrobots";

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

Environment getEnvironmentFromString(const std::string& envStr) {
    if (envStr == "Staging") return Environment::Staging;
    else if (envStr == "Production") return Environment::Production;
   return Environment::LocalDev;
}

const Environment environment = getEnvironmentFromString("LocalDev");

const char* getServerUrl() {
    if (environment == Environment::LocalDev) {
        return "http://192.168.145.40:8080";
    } else if (environment == Environment::Staging) {  // Assume Staging for any other environment
        return "staging-api.bluedotrobots.com"; // HTTP/HTTPS prefix handled at usage level if needed
    }
    return "prod-api.bluedotrobots.com";
}

const char* getWsServerUrl() {
    if (environment == Environment::LocalDev) {
        return "ws://192.168.145.40:8080/esp32";
    } else if (environment == Environment::Staging) {  // Assume Staging for any other environment
        return "wss://staging-api.bluedotrobots.com/esp32";
    }
    return "wss://prod-api.bluedotrobots.com/esp32";
}
