#include "demo_manager.h"

bool DemoManager::startDemo(Demo::DemoType demoType) {
    // Don't start the same demo that's already running
    if (_currentDemo == demoType && demoType != Demo::DemoType::NONE) {
        Serial.printf("Demo %s is already running\n", getDemoName(demoType));
        return true;
    }
    
    // Stop current demo if running
    if (_currentDemo != Demo::DemoType::NONE) {
        Serial.printf("Stopping current demo: %s\n", getDemoName(_currentDemo));
        disableCurrentDemo();
        
        // Small delay to ensure clean transition
        if (millis() - _lastTransitionTime < TRANSITION_DELAY_MS) {
            delay(TRANSITION_DELAY_MS);
        }
    }
    
    // Store previous demo for debugging
    _previousDemo = _currentDemo;
    
    // Start the new demo
    if (demoType == Demo::DemoType::NONE) {
        _currentDemo = Demo::DemoType::NONE;
        Serial.println("All demos stopped");
        return true;
    }
    
    bool success = enableDemo(demoType);
    if (success) {
        _currentDemo = demoType;
        _lastTransitionTime = millis();
        Serial.printf("Demo started: %s\n", getDemoName(demoType));
    } else {
        _currentDemo = Demo::DemoType::NONE;
        Serial.printf("Failed to start demo: %s\n", getDemoName(demoType));
    }
    
    return success;
}

void DemoManager::stopCurrentDemo() {
    if (_currentDemo != Demo::DemoType::NONE) {
        Serial.printf("Stopping demo: %s\n", getDemoName(_currentDemo));
        disableCurrentDemo();
        _previousDemo = _currentDemo;
        _currentDemo = Demo::DemoType::NONE;
        _lastTransitionTime = millis();
    }
}

void DemoManager::update() {
    // Only update if we have an active demo
    if (_currentDemo == Demo::DemoType::NONE) {
        return;
    }
    
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
            Serial.printf("Unknown demo type in update: %d\n", static_cast<int>(_currentDemo));
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
            Serial.printf("Unknown demo type in disable: %d\n", static_cast<int>(_currentDemo));
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
            Serial.printf("Unknown demo type in enable: %d\n", static_cast<int>(demoType));
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
