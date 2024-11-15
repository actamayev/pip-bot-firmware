#include <Arduino.h>
#include "./include/user_code.h"
#include "./include/config.h"

void user_code() {
while(true) {
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(2000);
}
}
