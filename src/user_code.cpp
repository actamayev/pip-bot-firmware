#include "./include/config.h"
#include "./include/rgb_led.h"
#include "./include/user_code.h"

void user_code() {
while(true) {
    rgbLed.set_led_white();
    delay(1000);
}
}
