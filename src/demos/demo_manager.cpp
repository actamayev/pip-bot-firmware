#include "demo_manager.h"

bool DemoManager::start_demo(demo::DemoType demo_type) {
    // Don't start the same demo that's already running
    if (_currentDemo == demo_type && demo_type != demo::DemoType::NONE) {
        return true;
    }

    // Stop current demo if running
    if (_currentDemo != demo::DemoType::NONE) {
        disable_current_demo();
    }
    if (demo_type == demo::DemoType::NONE) {
        _currentDemo = demo::DemoType::NONE;
        return true;
    }

    bool success = enable_demo(demo_type);
    if (success) {
        _currentDemo = demo_type;
    } else {
        _currentDemo = demo::DemoType::NONE;
    }

    return success;
}

void DemoManager::stop_current_demo() {
    if (_currentDemo == demo::DemoType::NONE) {
        return;
    }
    disable_current_demo();
    _currentDemo = demo::DemoType::NONE;
}

void DemoManager::update() {
    // Only update if we have an active demo
    if (_currentDemo == demo::DemoType::NONE) {
        return;
    }

    // Call the appropriate demo's update method
    switch (_currentDemo) {
        case demo::DemoType::BALANCE_CONTROLLER:
            BalanceController::get_instance().update();
            break;

        case demo::DemoType::OBSTACLE_AVOIDER:
            ObstacleAvoider::get_instance().update();
            break;

        case demo::DemoType::NONE:
            // Should never reach here, but handle gracefully
            break;

        default:
            stop_current_demo(); // Stop unknown demo
            break;
    }
}

void DemoManager::disable_current_demo() {
    switch (_currentDemo) {
        case demo::DemoType::BALANCE_CONTROLLER:
            BalanceController::get_instance().disable();
            break;

        case demo::DemoType::OBSTACLE_AVOIDER:
            ObstacleAvoider::get_instance().disable();
            break;

        case demo::DemoType::NONE:
            // Nothing to disable
            break;

        default:
            break;
    }
}

bool DemoManager::enable_demo(demo::DemoType demo_type) {
    switch (demo_type) {
        case demo::DemoType::BALANCE_CONTROLLER:
            BalanceController::get_instance().enable();
            return BalanceController::get_instance().is_enabled();

        case demo::DemoType::OBSTACLE_AVOIDER:
            ObstacleAvoider::get_instance().enable();
            return ObstacleAvoider::get_instance().is_enabled();

        case demo::DemoType::NONE:
            return true; // "Enabling" none is always successful

        default:
            return false;
    }
}

const char* DemoManager::get_demo_name(demo::DemoType demo_type) {
    switch (demo_type) {
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
