#pragma once
#include <Adafruit_BNO08x.h>

extern void quaternionToEuler(float qr, float qi, float qj, float qk, float& yaw, float& pitch, float& roll);
