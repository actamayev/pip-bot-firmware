#include "obstacle_avoider.h"

void ObstacleAvoider::enable() {
    if (_avoidanceEnabled == ObstacleAvoidanceStatus::AVOID) return;
    
    _avoidanceEnabled = ObstacleAvoidanceStatus::AVOID;
    StraightLineDrive::getInstance().disable();

    SerialQueueManager::getInstance().queueMessage("Obstacle Avoidance mode enabled");
}

void ObstacleAvoider::disable() {
    if (_avoidanceEnabled == ObstacleAvoidanceStatus::STOP_AVOIDANCE) return;
    _avoidanceEnabled = ObstacleAvoidanceStatus::STOP_AVOIDANCE;
    motorDriver.brake_if_moving();
    SerialQueueManager::getInstance().queueMessage("Obstacle Avoidance mode disabled");
}

void ObstacleAvoider::update() {
    if (_avoidanceEnabled != ObstacleAvoidanceStatus::AVOID) return;

    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < UPDATE_INTERVAL) {
        return; // Maintain update rate
    }
    _lastUpdateTime = currentTime;
}
