#pragma once
#include <Wire.h>
#include <ArduinoJson.h>
#include "structs.h"
#include "utils/preferences_manager.h"
#include "networking/serial_queue_manager.h"

void quaternionToEuler(float qr, float qi, float qj, float qk, float& yaw, float& pitch, float& roll);
bool check_address_on_i2c_line(uint8_t addr);
void scanI2C();
float calculateCircularMean(const float angles[], uint8_t count);

const char* routeToStringCommon(ToCommonMessage route);
const char* routeToStringServer(ToServerMessage route);
const char* routeToStringSerial(ToSerialMessage route);


template <size_t N>
StaticJsonDocument<N> makeBaseMessageCommon(ToCommonMessage route) {
    StaticJsonDocument<N> doc;
    doc["route"] = routeToStringCommon(route);
    return doc;
}

template <size_t N>
StaticJsonDocument<N> makeBaseMessageServer(ToServerMessage route) {
    StaticJsonDocument<N> doc;
    doc["route"] = routeToStringServer(route);
    return doc;
}

template <size_t N>
StaticJsonDocument<N> makeBaseMessageSerial(ToSerialMessage route) {
    StaticJsonDocument<N> doc;
    doc["route"] = routeToStringSerial(route);
    return doc;
}
