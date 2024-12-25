#pragma once
#include <Adafruit_BNO08x.h>

extern void quaternionToEuler(float qr, float qi, float qj, float qk, float& yaw, float& pitch, float& roll);
extern bool check_address_on_i2c_line(uint8_t addr);
