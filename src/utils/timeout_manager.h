#pragma once
#include <Arduino.h>
#include "utils/singleton.h"

class TimeoutManager : public Singleton<TimeoutManager> {
    friend class Singleton<TimeoutManager>;

    public:
        void resetActivity();
        void update();
        bool isInConfirmationState() const { return inConfirmationState; }
        void cancelConfirmation();

    private:
        TimeoutManager() = default;
        
        unsigned long lastActivityTime = 0;
        bool inConfirmationState = false;
        unsigned long confirmationStartTime = 0;
        
        static constexpr unsigned long INACTIVITY_TIMEOUT = 10 * 1000; // 15 seconds
        static constexpr unsigned long CONFIRMATION_TIMEOUT = 10 * 1000; // 1 seconds
        
        void enterConfirmationState();
        void enterDeepSleep();
};
