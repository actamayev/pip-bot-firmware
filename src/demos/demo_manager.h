#pragma once

#include <Arduino.h>
#include "utils/singleton.h"
#include "utils/structs.h"
#include "demos/balance_controller.h"
#include "demos/obstacle_avoider.h"

class DemoManager : public Singleton<DemoManager> {
    friend class Singleton<DemoManager>;

    public:
        // Start a specific demo (automatically stops current demo if running)
        bool startDemo(Demo::DemoType demoType);
        
        // Stop the currently running demo
        void stopCurrentDemo();
        
        // Update the currently running demo (call this in your main loop)
        void update();
        
        // Get the currently active demo
        Demo::DemoType getCurrentDemo() const { return _currentDemo; }
        
        // Check if any demo is currently active
        bool isAnyDemoActive() const { return _currentDemo != Demo::DemoType::NONE; }
        
        // Get demo name as string (useful for debugging)
        const char* getDemoName(Demo::DemoType demoType) const;

    private:
        DemoManager() = default;
        
        // Internal methods
        void disableCurrentDemo();
        bool enableDemo(Demo::DemoType demoType);
        
        // State
        Demo::DemoType _currentDemo = Demo::DemoType::NONE;
        Demo::DemoType _previousDemo = Demo::DemoType::NONE;
};
