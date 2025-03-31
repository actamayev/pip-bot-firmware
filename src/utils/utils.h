#pragma once
#include <WebServer.h>
#include <ArduinoJson.h>
#include <mbedtls/base64.h>
#include <Adafruit_BNO08x.h>
#include "./structs.h"

extern void quaternionToEuler(float qr, float qi, float qj, float qk, float& yaw, float& pitch, float& roll);
extern bool check_address_on_i2c_line(uint8_t addr);
extern void scanI2C();
float calculateCircularMean(const float angles[], uint8_t count);

