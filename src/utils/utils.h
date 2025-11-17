#pragma once
#include <ArduinoJson.h>
#include <Wire.h>

#include "networking/serial_queue_manager.h"
#include "structs.h"
#include "utils/preferences_manager.h"

void quaternion_to_euler(float qr, float qi, float qj, float qk, float& yaw, float& pitch, float& roll);
bool check_address_on_i2c_line(uint8_t addr);
void scan_i2_c();
float calculate_circular_mean(const float angles[], uint8_t count);

const char* route_to_string_common(ToCommonMessage route);
const char* route_to_string_server(ToServerMessage route);
const char* route_to_string_serial(ToSerialMessage route);

template <size_t N> StaticJsonDocument<N> make_base_message_common(ToCommonMessage route) {
    StaticJsonDocument<N> doc;
    doc["route"] = route_to_string_common(route);
    return doc;
}

template <size_t N> StaticJsonDocument<N> make_base_message_server(ToServerMessage route) {
    StaticJsonDocument<N> doc;
    doc["route"] = route_to_string_server(route);
    return doc;
}

template <size_t N> StaticJsonDocument<N> make_base_message_serial(ToSerialMessage route) {
    StaticJsonDocument<N> doc;
    doc["route"] = route_to_string_serial(route);
    return doc;
}
