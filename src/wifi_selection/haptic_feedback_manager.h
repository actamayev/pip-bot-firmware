#pragma once
#include <Arduino.h>
#include "../utils/singleton.h"

class HapticFeedbackManager : public Singleton<HapticFeedbackManager> {
    friend class Singleton<HapticFeedbackManager>;

    public:
        // Public API for haptic feedback
        void startFeedback(int8_t direction, uint8_t strength, uint8_t duration_ms);
        void update();

        // Haptic states
        enum HapticState {
            HAPTIC_IDLE,
            HAPTIC_RESISTANCE,      // Stage 1: Initial resistance against movement
            HAPTIC_REVERSE_PULSE,   // Stage 2: Helping pulse in direction of movement
            HAPTIC_CENTERING,       // Stage 3: Gentle centering force
            HAPTIC_FINAL_BUMP,      // Stage 4: Final "click" into place
            HAPTIC_RECOVERY         // Cool-down period before accepting new input
        };
        
        // Getters
        bool isActive() const { return _hapticState != HAPTIC_IDLE; }
        
    private:
        // Constructor
        HapticFeedbackManager() = default;
        
        // State variables
        HapticState _hapticState = HAPTIC_IDLE;
        int8_t _hapticDirection = 0;
        uint8_t _resistanceStrength = 0;
        uint8_t _reversePulseStrength = 0;
        uint8_t _centeringStrength = 0;
        uint8_t _finalBumpStrength = 0;
        unsigned long _hapticStartTime = 0;
        unsigned long _resistanceDuration = 0;
        unsigned long _reversePulseDuration = 0;
        unsigned long _centeringDuration = 0;
        unsigned long _finalBumpDuration = 0;
        unsigned long _recoveryDuration = 0;
};
