#include "./include/config.h"
#include "./include/rgb_led.h"
#include "./include/user_code.h"
#include "./include/sensors.h"
#include "./include/utils.h"

void user_code() {
    const auto& angles = Sensors::getInstance().getEulerAngles();

    if (!angles.isValid) {
        rgbLed.turn_led_off();
    } else {
        if (angles.pitch >= 0 && angles.pitch <= 10) {
            rgbLed.set_led_blue();  // Level position
        } else if (angles.pitch > 10) {
            rgbLed.set_led_red();   // Tilted up
        } else {
            rgbLed.set_led_green(); // Tilted down
        }
    }

    delay(50);
}
