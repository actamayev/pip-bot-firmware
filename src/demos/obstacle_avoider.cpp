#include "obstacle_avoider.h"

void ObstacleAvoider::enable() {
    if (_avoidanceEnabled == ObstacleAvoidanceStatus::AVOID) return;
    
    _avoidanceEnabled = ObstacleAvoidanceStatus::AVOID;
    // StraightLineDrive::getInstance().disable();
}

void ObstacleAvoider::disable() {
    if (_avoidanceEnabled == ObstacleAvoidanceStatus::STOP_AVOIDANCE) return;
    _avoidanceEnabled = ObstacleAvoidanceStatus::STOP_AVOIDANCE;
    motorDriver.resetCommandState();
}

void ObstacleAvoider::update() {
    if (_avoidanceEnabled != ObstacleAvoidanceStatus::AVOID) return;

    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < UPDATE_INTERVAL) {
        return; // Maintain update rate
    }
    _lastUpdateTime = currentTime;
}
