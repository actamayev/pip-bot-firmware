#include "./include/config.h"
#include "./include/lab_demo_manager.h"
#include "./include/rgb_led.h"

void LabDemoManager::handleLabMessage(const char* json, int tokenCount, jsmntok_t* tokens) {
    // Find the "event" field in the JSON
    for (int i = 1; i < tokenCount; i += 2) {
        String key = String(json + tokens[i].start, tokens[i].end - tokens[i].start);
        if (key == "event") {
            String eventType = String(json + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
            
            if (eventType == "motor-control-data") {
                handleMotorControl(json, tokenCount, tokens);
                break;
            }
        }
    }
}

void LabDemoManager::handleMotorControl(const char* json, int tokenCount, jsmntok_t* tokens) {
    int leftMotor = 0;
    int rightMotor = 0;

    // Extract motor control values
    for (int j = 1; j < tokenCount; j += 2) {
        String fieldKey = String(json + tokens[j].start, tokens[j].end - tokens[j].start);
        if (fieldKey == "leftMotor") {
            leftMotor = extractInt(json, &tokens[j + 1]);
        } else if (fieldKey == "rightMotor") {
            rightMotor = extractInt(json, &tokens[j + 1]);
        }
    }

    updateMotorSpeeds(leftMotor, rightMotor);
}

void LabDemoManager::updateMotorSpeeds(int leftMotor, int rightMotor) {
    Serial.printf("Motors updated - Left: %d, Right: %d\n", leftMotor, rightMotor);

    if (leftMotor == -1 && rightMotor == -1) {
        rgbLed.set_led_red();
        return;
    } else if (leftMotor == -1 && rightMotor == 1) {
        rgbLed.set_led_white();
        return;
    } else if (leftMotor == 1 && rightMotor == -1) {
        rgbLed.set_led_green();
        return;
    } else if (leftMotor == 1 && rightMotor == 1) {
        rgbLed.set_led_blue();
    } else {
        rgbLed.turn_led_off();
    }
    // // Ensure values are within PWM range (0-255)
    // leftMotor = constrain(leftMotor, 0, 255);
    // rightMotor = constrain(rightMotor, 0, 255);

    // // Update motor speeds
    // analogWrite(LEFT_MOTOR_PIN, leftMotor);
    // analogWrite(RIGHT_MOTOR_PIN, rightMotor);
    
    // Serial.printf("Motors updated - Left: %d, Right: %d\n", leftMotor, rightMotor);
}

int64_t LabDemoManager::extractInt(const char* json, const jsmntok_t* tok) {
    char numStr[32];
    int len = min(31, tok->end - tok->start);
    strncpy(numStr, json + tok->start, len);
    numStr[len] = '\0';
    return atoll(numStr);
}
