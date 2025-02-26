#pragma once

#include <Arduino.h>
#include "./singleton.h"
#include "./structs.h"

class LabDemoManager : public Singleton<LabDemoManager> {
    friend class Singleton<LabDemoManager>;

    public:
        void handleBinaryMessage(const char* data);
        void updateMotorSpeeds(int16_t leftSpeed, int16_t rightSpeed);
        void monitorEncoders();
        
        // Method to send encoder data to server
        void sendEncoderDataToServer(float leftWheelRPM, float rightWheelRPM);

    private:
        LabDemoManager() {}
};
