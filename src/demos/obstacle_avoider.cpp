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
    motorDriver.reset_command_state(false);
}

void ObstacleAvoider::update() {
    if (_avoidanceEnabled != ObstacleAvoidanceStatus::AVOID) {
        return;
    }

    unsigned long current_time = millis();
    if (current_time - _lastUpdateTime < _UPDATE_INTERVAL) {
        return; // Maintain update rate
    }
    _lastUpdateTime = current_time;
}
