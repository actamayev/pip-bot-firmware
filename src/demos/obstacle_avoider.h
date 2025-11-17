#pragma once

#include <Arduino.h>
#include "utils/utils.h"
#include "utils/singleton.h"
#include "networking/protocol.h"
#include "actuators/led/rgb_led.h"
#include "actuators/motor_driver.h"

class ObstacleAvoider : public Singleton<ObstacleAvoider> {
    friend class Singleton<ObstacleAvoider>;

    public:
        void enable();
        void disable();
        void update();
        bool is_enabled() const { return _avoidanceEnabled == ObstacleAvoidanceStatus::AVOID; }

    private:
        ObstacleAvoider() = default;

        // State
        ObstacleAvoidanceStatus _avoidanceEnabled = ObstacleAvoidanceStatus::STOP_AVOIDANCE;

        unsigned long _lastUpdateTime = 0;
        unsigned long _UPDATE_INTERVAL = 3; // 3ms (333Hz)
};
