#pragma once

#include <Arduino.h>

#include "actuators/led/rgb_led.h"
#include "actuators/motor_driver.h"
#include "networking/protocol.h"
#include "utils/singleton.h"
#include "utils/utils.h"

class ObstacleAvoider : public Singleton<ObstacleAvoider> {
    friend class Singleton<ObstacleAvoider>;

  public:
    void enable();
    void disable();
    void update();
    bool is_enabled() const {
        return _avoidanceEnabled == ObstacleAvoidanceStatus::AVOID;
    }

  private:
    ObstacleAvoider() = default;

    // State
    ObstacleAvoidanceStatus _avoidanceEnabled = ObstacleAvoidanceStatus::STOP_AVOIDANCE;

    uint32_t _lastUpdateTime = 0;
    uint32_t _UPDATE_INTERVAL = 3; // 3ms (333Hz)
};
