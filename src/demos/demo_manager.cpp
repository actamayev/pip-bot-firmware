#include "demo_manager.h"

bool DemoManager::startDemo(demo::DemoType demoType) {
    // Don't start the same demo that's already running
    if (_currentDemo == demoType && demoType != demo::DemoType::NONE) {
        return true;
    }

    // Stop current demo if running
    if (_currentDemo != demo::DemoType::NONE) {
        disableCurrentDemo();
    }
    if (demoType == demo::DemoType::NONE) {
        _currentDemo = demo::DemoType::NONE;
        return true;
    }

    bool success = enableDemo(demoType);
    if (success) {
        _currentDemo = demoType;
    } else {
        _currentDemo = demo::DemoType::NONE;
    }

    return success;
}

void DemoManager::stopCurrentDemo() {
    if (_currentDemo == demo::DemoType::NONE) return;
    disableCurrentDemo();
    _currentDemo = demo::DemoType::NONE;
}

void DemoManager::update() {
    // Only update if we have an active demo
    if (_currentDemo == demo::DemoType::NONE) return;

    // Call the appropriate demo's update method
    switch (_currentDemo) {
        case demo::DemoType::BALANCE_CONTROLLER:
            BalanceController::getInstance().update();
            break;

        case demo::DemoType::OBSTACLE_AVOIDER:
            ObstacleAvoider::getInstance().update();
            break;

        case demo::DemoType::NONE:
            // Should never reach here, but handle gracefully
            break;

        default:
            stopCurrentDemo(); // Stop unknown demo
            break;
    }
}

void DemoManager::disableCurrentDemo() {
    switch (_currentDemo) {
        case demo::DemoType::BALANCE_CONTROLLER:
            BalanceController::getInstance().disable();
            break;

        case demo::DemoType::OBSTACLE_AVOIDER:
            ObstacleAvoider::getInstance().disable();
            break;

        case demo::DemoType::NONE:
            // Nothing to disable
            break;

        default:
            break;
    }
}

bool DemoManager::enableDemo(demo::DemoType demoType) {
    switch (demoType) {
        case demo::DemoType::BALANCE_CONTROLLER:
            BalanceController::getInstance().enable();
            return BalanceController::getInstance().isEnabled();

        case demo::DemoType::OBSTACLE_AVOIDER:
            ObstacleAvoider::getInstance().enable();
            return ObstacleAvoider::getInstance().isEnabled();

        case demo::DemoType::NONE:
            return true; // "Enabling" none is always successful

        default:
            return false;
    }
}

const char* DemoManager::getDemoName(demo::DemoType demoType) const {
    switch (demoType) {
        case demo::DemoType::NONE:
            return "None";
        case demo::DemoType::BALANCE_CONTROLLER:
            return "Balance Controller";
        case demo::DemoType::OBSTACLE_AVOIDER:
            return "Obstacle Avoider";
        default:
            return "Unknown";
    }
}
