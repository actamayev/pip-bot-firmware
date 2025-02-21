#pragma once

#include <Arduino.h>
#include "./singleton.h"
#include "./jsmn.h"

class LabDemoManager : public Singleton<LabDemoManager> {
    friend class Singleton<LabDemoManager>;

    public:
        void handleBinaryMessage(const char* data, size_t length);
        void updateMotorSpeeds(int leftMotor, int rightMotor);

    private:
        LabDemoManager() {}

        int byteToMotorSpeed(uint8_t value) {
            switch (value) {
                case 1: return -1;  // 0001
                case 2: return 0;   // 0010
                case 3: return 1;   // 0011
                default: return 0;
            }
        }
};
