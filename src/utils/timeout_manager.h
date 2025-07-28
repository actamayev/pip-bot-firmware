#pragma once
#include <Arduino.h>
#include "utils/singleton.h"

class TimeoutManager : public Singleton<TimeoutManager> {
    friend class Singleton<TimeoutManager>;

    public:
        void resetActivity();
        void update();
        bool isInConfirmationState() const { return inConfirmationState; }

    private:
        TimeoutManager() = default;
        
        unsigned long lastActivityTime = 0;
        bool inConfirmationState = false;
        unsigned long confirmationStartTime = 0;

        static constexpr unsigned long INACTIVITY_TIMEOUT = 9 * 60 * 1000; // 9 minutes
        static constexpr unsigned long CONFIRMATION_TIMEOUT = 1 * 60 * 1000; // 1 minute

        void cancelConfirmation();
        void enterConfirmationState();
        void enterDeepSleep();
};
