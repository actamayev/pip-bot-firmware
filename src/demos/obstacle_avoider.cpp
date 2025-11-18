#include "obstacle_avoider.h"

void ObstacleAvoider::enable() {
    if (_avoidanceEnabled == ObstacleAvoidanceStatus::AVOID) {
        return;
    }

    _avoidanceEnabled = ObstacleAvoidanceStatus::AVOID;
    // StraightLineDrive::get_instance().disable();
}

void ObstacleAvoider::disable() {
    if (_avoidanceEnabled == ObstacleAvoidanceStatus::STOP_AVOIDANCE) {
        return;
    }
    _avoidanceEnabled = ObstacleAvoidanceStatus::STOP_AVOIDANCE;
    motor_driver.reset_command_state(false);
}

void ObstacleAvoider::update() {
    if (_avoidanceEnabled != ObstacleAvoidanceStatus::AVOID) {
        return;
    }

    const uint32_t CURRENT_TIME = millis();
    if (CURRENT_TIME - _lastUpdateTime < UPDATE_INTERVAL) {
        return; // Maintain update rate
    }
    _lastUpdateTime = CURRENT_TIME;
}
