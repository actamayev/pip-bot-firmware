#pragma once
#include "Arduino.h"
#include "utils/structs.h"
#include "utils/singleton.h"

class IrSensor : public Singleton<IrSensor> {
    friend class Singleton<IrSensor>;

    public:
        IrSensor();
        float* getSensorData();

    private:
        void read_ir_sensor();
        void setMuxChannel(bool A0, bool A1, bool A2);

        static constexpr MuxChannel channels[5] = {
            {"S5", LOW, LOW, HIGH},
            {"S6", HIGH, LOW, HIGH},
            {"S4", LOW, HIGH, HIGH},
            {"S8", HIGH, HIGH, HIGH},
            {"S7", HIGH, HIGH, LOW},
        };
        const float cutoff = 1.75f;
        float sensorReadings[5];
};
