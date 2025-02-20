#pragma once

#include <Arduino.h>
#include "./singleton.h"
#include "./jsmn.h"

class LabDemoManager : public Singleton<LabDemoManager> {
    friend class Singleton<LabDemoManager>;

    public:
        void handleLabMessage(const char* json, int tokenCount, jsmntok_t* tokens);
        void updateMotorSpeeds(int leftMotor, int rightMotor);

    private:
        LabDemoManager() {}
        
        void handleMotorControl(const char* json, int tokenCount, jsmntok_t* tokens);
        int64_t extractInt(const char* json, const jsmntok_t* tok);
};
