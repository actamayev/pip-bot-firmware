#pragma once
#include <Arduino.h>
#include "sensors/sensor_data_buffer.h"
#include "actuators/motor_driver.h"

enum class TurningDirection {
    NONE,
    CLOCKWISE,
    COUNTER_CLOCKWISE
};

enum class TurningState {
    IDLE,
    TURNING
};

class TurningManager : public Singleton<TurningManager> {
    friend class Singleton<TurningManager>;

    public:
        // Main interface
        bool startTurn(float degrees);
        void update();
        void completeNavigation();
        const bool isActive() { return currentState == TurningState::TURNING; };

        // Debug info structure
        struct DebugInfo {
            float targetAngle = 0.0f;
            float cumulativeRotation = 0.0f;
            float currentError = 0.0f;
            float currentVelocity = 0.0f;
            uint8_t currentMinPWM = 0;
            uint8_t currentMaxPWM = 0;
            uint8_t directionChanges = 0;
            bool inSafetyPause = false;
        };
        const DebugInfo& getDebugInfo() const { return _debugInfo; }

    private:
        TurningManager() = default;
        ~TurningManager() = default;

        // Core turning logic
        void initializeTurn();
        void updateCumulativeRotation();
        void getRotationError();
        void updateVelocity();
        uint8_t calculatePIDSpeed();
        void adaptPWMLimits(uint8_t commandedPWM);
        
        // Safety system
        bool checkSafetyTriggers();
        void triggerSafetyPause();
        
        // Motor control helpers
        void startTurnMotors();
        void setTurnSpeed(uint8_t speed);
        
        // State variables
        TurningState currentState = TurningState::IDLE;
        TurningDirection currentDirection = TurningDirection::NONE;
        TurningDirection targetDirection = TurningDirection::NONE;
        
        float TARGET_TURN_ANGLE = 0;
        // Turn parameters
        float cumulativeRotation = 0.0f;
        float lastHeadingForRotation = 0.0f;
        bool rotationTrackingInitialized = false;
        
        // Control parameters
        float currentError = 0.0f;
        float currentVelocity = 0.0f;
        float lastHeading = 0.0f;
        unsigned long lastTime = 0;
        
        // Adaptive PWM limits
        uint8_t currentMaxPWM = 60;
        uint8_t currentMinPWM = 25;
        float targetMaxVelocity = 400.0f;
        float targetMinVelocity = 40.0f;
        unsigned long lastAdaptationTime = 0;
        
        // Completion tracking
        unsigned long turnCompletionStartTime = 0;
        bool turnCompletionConfirmed = false;
        
        // Safety system
        uint8_t turnDirectionChanges = 0;
        unsigned long safetyPauseStartTime = 0;
        bool inSafetyPause = false;
        uint8_t safetyDefaultMinPWM = 25;
        uint8_t safetyDefaultMaxPWM = 45;
        
        // Constants
        static constexpr float DEAD_ZONE = 1.0f;
        static constexpr unsigned long COMPLETION_CONFIRMATION_TIME = 250; // ms
        static constexpr unsigned long SAFETY_PAUSE_DURATION = 50; // ms
        static constexpr uint8_t MAX_DIRECTION_CHANGES = 3;
        static constexpr unsigned long ADAPTATION_RATE_LIMIT = 20; // ms

        // Debug info
        mutable DebugInfo _debugInfo;

        bool stictionDetected = false;
        uint8_t stictionBoostLevel = 0;
        unsigned long stictionDetectionStartTime = 0;
        static constexpr unsigned long STICTION_DETECTION_TIME = 200; // ms
        static constexpr float STICTION_VELOCITY_THRESHOLD = 5.0f; // degrees/sec

        // Add these methods to private section
        void detectStiction();
        void resetStictionDetection();
};
