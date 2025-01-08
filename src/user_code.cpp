#include "./include/config.h"
#include "./include/rgb_led.h"
#include "./include/user_code.h"
#include "./include/sensors.h"
#include "./include/utils.h"

void user_code() {
    float qX, qY, qZ, qW;
    const auto& quat = Sensors::getInstance().getQuaternion();

    if (!quat.isValid) {
        rgbLed.turn_led_off();
    } else {
        float yaw, pitch, roll;
        quaternionToEuler(quat.qW, quat.qX, quat.qY, quat.qZ, yaw, pitch, roll);

        if (pitch >= 0 && pitch <= 10) {
            rgbLed.set_led_blue();  // Level position
        } else if (pitch > 10) {
            rgbLed.set_led_red();   // Tilted up
        } else {
            rgbLed.set_led_green(); // Tilted down
        }
    }

    // Small delay to prevent too frequent updates
    delay(50);
}
