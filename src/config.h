#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

extern const uint8_t LED_PIN;
extern const uint8_t DNS_PORT;

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
