#include "demo_manager.h"

bool DemoManager::startDemo(Demo::DemoType demoType) {
    // Don't start the same demo that's already running
    if (_currentDemo == demoType && demoType != Demo::DemoType::NONE) {
        return true;
    }

    // Stop current demo if running
    if (_currentDemo != Demo::DemoType::NONE) {
        disableCurrentDemo();
    }
    if (demoType == Demo::DemoType::NONE) {
        _currentDemo = Demo::DemoType::NONE;
        return true;
    }
    
    bool success = enableDemo(demoType);
    if (success) {
        _currentDemo = demoType;
    } else {
        _currentDemo = Demo::DemoType::NONE;
    }
    
    return success;
}

void DemoManager::stopCurrentDemo() {
    if (_currentDemo == Demo::DemoType::NONE) return;
    disableCurrentDemo();
    _currentDemo = Demo::DemoType::NONE;
}

void DemoManager::update() {
    // Only update if we have an active demo
    if (_currentDemo == Demo::DemoType::NONE) return;

    // Call the appropriate demo's update method
    switch (_currentDemo) {
        case Demo::DemoType::BALANCE_CONTROLLER:
            BalanceController::getInstance().update();
            break;
            
        case Demo::DemoType::OBSTACLE_AVOIDER:
            ObstacleAvoider::getInstance().update();
            break;
            
        case Demo::DemoType::NONE:
            // Should never reach here, but handle gracefully
            break;
            
        default:
            stopCurrentDemo(); // Stop unknown demo
            break;
    }
}

void DemoManager::disableCurrentDemo() {
    switch (_currentDemo) {
        case Demo::DemoType::BALANCE_CONTROLLER:
            BalanceController::getInstance().disable();
            break;
            
        case Demo::DemoType::OBSTACLE_AVOIDER:
            ObstacleAvoider::getInstance().disable();
            break;
            
        case Demo::DemoType::NONE:
            // Nothing to disable
            break;
            
        default:
            break;
    }
}

bool DemoManager::enableDemo(Demo::DemoType demoType) {
    switch (demoType) {
        case Demo::DemoType::BALANCE_CONTROLLER:
            BalanceController::getInstance().enable();
            return BalanceController::getInstance().isEnabled();
            
        case Demo::DemoType::OBSTACLE_AVOIDER:
            ObstacleAvoider::getInstance().enable();
            return ObstacleAvoider::getInstance().isEnabled();

        case Demo::DemoType::NONE:
            return true; // "Enabling" none is always successful
            
        default:
            return false;
    }
}

const char* DemoManager::getDemoName(Demo::DemoType demoType) const {
    switch (demoType) {
        case Demo::DemoType::NONE:
            return "None";
        case Demo::DemoType::BALANCE_CONTROLLER:
            return "Balance Controller";
        case Demo::DemoType::OBSTACLE_AVOIDER:
            return "Obstacle Avoider";
        default:
            return "Unknown";
    }
}
