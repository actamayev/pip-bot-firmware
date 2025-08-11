#pragma once
#include "Arduino.h"
#include "utils/structs.h"
#include "utils/singleton.h"
#include "sensors/sensor_data_buffer.h"

class IrSensor : public Singleton<IrSensor> {
    friend class Singleton<IrSensor>;
    friend class TaskManager;

    public:
        IrSensor();

    private:
        void read_ir_sensor();
        void setMuxChannel(bool A0, bool A1, bool A2);

        const MuxChannel channels[5] = {
            {"S5", LOW, LOW, HIGH},
            {"S6", HIGH, LOW, HIGH},
            {"S4", LOW, HIGH, HIGH},
            {"S8", HIGH, HIGH, HIGH},
            {"S7", HIGH, HIGH, LOW},
        };
        const float cutoff = 1.75f;
        float sensorReadings[5];

        bool sensorEnabled = false;  // Track if sensor is actively enabled
        void enableIrSensor();
        void disableIrSensor();
        unsigned long lastUpdateTime = 0;
        static constexpr unsigned long DELAY_BETWEEN_READINGS = 20; // ms - rate limiting

        // Task Manager Methods:
        void updateSensorData();  // Single read, write to buffer
        bool shouldBePolling() const;
};
