#pragma once
#include <Wire.h>
#include "structs.h"
#include "networking/serial_queue_manager.h"

void quaternionToEuler(float qr, float qi, float qj, float qk, float& yaw, float& pitch, float& roll);
bool check_address_on_i2c_line(uint8_t addr);
void scanI2C();
float calculateCircularMean(const float angles[], uint8_t count);
const char* routeToString(RouteType route);
