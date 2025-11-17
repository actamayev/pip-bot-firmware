#pragma once

#include <Arduino.h>

#include "actuators/led/rgb_led.h"
#include "actuators/motor_driver.h"
#include "demos/demo_manager.h"
#include "networking/protocol.h"
#include "networking/serial_queue_manager.h"
#include "sensors/imu.h"
#include "utils/singleton.h"
#include "utils/utils.h"

class BalanceController : public Singleton<BalanceController> {
    friend class Singleton<BalanceController>;

  public:
    void enable();
    void disable();
    void update();
    bool is_enabled() const {
        return _balancingEnabled == BalanceStatus::BALANCED;
    }
    void update_balance_pids(NewBalancePids new_balance_pids);

  private:
    BalanceController() = default;

    // State
    BalanceStatus _balancingEnabled = BalanceStatus::UNBALANCED;
    float _lastValidAngle = 0.0F;
    float _lastError = 0.0F;
    float _errorSum = 0.0F;
    uint32_t _lastUpdateTime = 0;

    // Fixed parameters - not configurable as per your request
    float TARGET_ANGLE = 93.6F; // Fixed target angle

    // PID Constants
    float P_GAIN = 28.5F;
    float I_GAIN = 0.0F;
    float D_GAIN = 0.0F;  // Previously 6.0f
    float FF_GAIN = 0.0F; // Not using this

    // Limits and safety parameters
    float MAX_SAFE_ANGLE_DEVIATION = 30.0F;
    uint32_t UPDATE_INTERVAL = 3; // 3ms (333Hz)

    // Filtering buffers
    static constexpr uint8_t ANGLE_BUFFER_SIZE = 5;
    float _angleBuffer[ANGLE_BUFFER_SIZE] = {0};
    uint8_t _angleBufferIndex = 0;
    uint8_t _angleBufferCount = 0;

    // Safety buffer for tilt monitoring
    float _safetyBuffer[ANGLE_BUFFER_SIZE] = {0};
    uint8_t _safetyBufferIndex = 0;
    uint8_t _safetyBufferCount = 0;

    float DEADBAND_ANGLE = 1.0F;
    float MAX_STABLE_ROTATION = 0.1F; // degrees/second

    float MIN_EFFECTIVE_PWM = 330;
};
