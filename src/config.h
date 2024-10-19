#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>  // or <cstdint> for C++

// Define your LED pin
#define LED_PIN 2
extern const uint8_t DNS_PORT;

// Declare your Access Point SSID and Password
extern const char* ap_ssid;
extern const char* ap_password;

// Declare WebSocket Server URL
extern const char* ws_server_url;

#endif
