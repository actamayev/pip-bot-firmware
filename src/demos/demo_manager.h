#pragma once

#include <Arduino.h>

#include "demos/balance_controller.h"
#include "demos/obstacle_avoider.h"
#include "networking/serial_queue_manager.h"
#include "utils/singleton.h"
#include "utils/structs.h"

class DemoManager : public Singleton<DemoManager> {
    friend class Singleton<DemoManager>;
    friend class BalanceController;

  public:
    // Start a specific demo (automatically stops current demo if running)
    bool startDemo(demo::DemoType demoType);

    // Stop the currently running demo
    void stopCurrentDemo();

    void update();

    demo::DemoType getCurrentDemo() const {
        return _currentDemo;
    }

    bool isAnyDemoActive() const {
        return _currentDemo != demo::DemoType::NONE;
    }

  private:
    DemoManager() = default;

    void disableCurrentDemo();
    bool enableDemo(demo::DemoType demoType);
    const char* getDemoName(demo::DemoType demoType) const;
    demo::DemoType _currentDemo = demo::DemoType::NONE;
};
