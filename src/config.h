#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

extern const uint8_t LED_PIN;
extern const uint8_t DNS_PORT;

extern const uint8_t DIGITAL_IR_PIN_1;
extern const uint8_t DIGITAL_IR_PIN_2;
extern const uint8_t DIGITAL_IR_PIN_3;

extern const char* ap_ssid;
extern const char* ap_password;

extern const char* ws_server_url;
extern const char* server_url;
extern const char* rootCACertificate;

enum class PathHeader {
    Auth
};

enum class PathFooter {
    Login,
    Logout
};

#endif
