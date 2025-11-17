#pragma once
#include <Arduino.h>

#include "utils/singleton.h"

class TimeoutManager : public Singleton<TimeoutManager> {
    friend class Singleton<TimeoutManager>;

  public:
    void reset_activity();
    void update();
    bool is_in_confirmation_state() const {
        return _inConfirmationState;
    }

  private:
    TimeoutManager() = default;

    uint32_t _lastActivityTime = 0;
    bool _inConfirmationState = false;
    uint32_t _confirmationStartTime = 0;

    static constexpr uint32_t INACTIVITY_TIMEOUT = 9 * 60 * 1000;   // 9 minutes
    static constexpr uint32_t CONFIRMATION_TIMEOUT = 1 * 60 * 1000; // 1 minute

    void cancel_confirmation();
    void enter_confirmation_state();
};
