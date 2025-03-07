#pragma once
#include "Arduino.h"
#include "../utils/structs.h"

class IrSensor {
    public:
        IrSensor();
        float* getSensorData();

    private:
        void read_ir_sensor();
        void setMuxChannel(bool A, bool B, bool C);

        MuxChannel channels[5];
        const float cutoff = 1.75f;
        float sensorReadings[5];
};

extern IrSensor irSensor;
