#pragma once

#include <Arduino.h>
#include "utils/singleton.h"
#include "utils/structs.h"
#include "demos/balance_controller.h"
#include "demos/obstacle_avoider.h"
#include "networking/serial_queue_manager.h"

class DemoManager : public Singleton<DemoManager> {
    friend class Singleton<DemoManager>;
    friend class BalanceController;

    public:
        // Start a specific demo (automatically stops current demo if running)
        bool startDemo(Demo::DemoType demoType);
        
        // Stop the currently running demo
        void stopCurrentDemo();
        
        void update();
        
        Demo::DemoType getCurrentDemo() const { return _currentDemo; }
        
        bool isAnyDemoActive() const { return _currentDemo != Demo::DemoType::NONE; }

    private:
        DemoManager() = default;
        
        void disableCurrentDemo();
        bool enableDemo(Demo::DemoType demoType);
        const char* getDemoName(Demo::DemoType demoType) const;
        Demo::DemoType _currentDemo = Demo::DemoType::NONE;
};
