#pragma once
#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <map>
#include <vector>

#include "actuators/buttons.h"
#include "actuators/led/rgb_led.h"
#include "actuators/motor_driver.h"
#include "actuators/speaker.h"
#include "bytecode_structs.h"
#include "demos/straight_line_drive.h"
#include "demos/turning_manager.h"
#include "networking/serial_manager.h"
#include "networking/serial_queue_manager.h"
#include "sensors/encoder_manager.h"
#include "sensors/imu.h"
#include "sensors/multizone_tof_sensor.h"
#include "sensors/sensor_data_buffer.h"
#include "utils/singleton.h"

class BytecodeVM : public Singleton<BytecodeVM> {
    friend class Singleton<BytecodeVM>;
    friend class Buttons;
    friend class TaskManager;

  public:
    // Load bytecode program into the VM
    static bool load_program(const uint8_t* byte_code, uint16_t size);
    static void stop_program();

    // Debug methods for distance movement
    bool is_distance_movement_active() const {
        return _distanceMovementInProgress;
    }
    float get_target_distance_in() const {
        return _targetDistanceIn;
    }
    float get_starting_distance_in() const {
        return _startingDistanceIn;
    }
    bool is_program_loaded() const {
        return _program != nullptr;
    }

  private:
    BytecodeVM();
    ~BytecodeVM();
    // Constants:
    static const uint16_t MAX_REGISTERS = 1024; // Changed from uint8_t to uint16_t
                                                // to handle values > 255

    static const uint8_t INSTRUCTION_SIZE = 20;

    static const uint16_t LEFT_PROXIMITY_THRESHOLD = 50;
    static const uint16_t RIGHT_PROXIMITY_THRESHOLD = 50;
    const int TURN_TIMEOUT = 2000; // 1 second timeout for turn operations

    BytecodeInstruction* _program = nullptr;
    uint16_t _programSize = 0;
    uint16_t _pc = 0;         // Program counter
    uint32_t _delayUntil = 0; // For handling delays
    bool _waitingForDelay = false;
    bool _lastComparisonResult = false; // Stores result of last comparison

    // Union to store different variable types in the same memory
    union RegisterValue {
        float asFloat;
        int32_t asInt;
        bool asBool;
        uint8_t asBytes[4];
    };

    RegisterValue _registers[MAX_REGISTERS]{};
    BytecodeVarType _registerTypes[MAX_REGISTERS]{};
    bool _registerInitialized[MAX_REGISTERS] = {false};

    // Update VM - call this regularly from main loop
    void update();

    enum PauseState { PROGRAM_NOT_STARTED, PAUSED, RUNNING, PROGRAM_FINISHED };

    PauseState _isPaused = PauseState::PROGRAM_NOT_STARTED;

    bool _waitingForButtonPressToStart = false;
    static bool can_start_program();

    // Execute instruction implementation
    static void execute_instruction(const BytecodeInstruction& instr);

    // Helper method for comparisons
    static bool compare_values(ComparisonOp op, float left_operand, float right_value);

    bool _timedMotorMovementInProgress = false;
    uint32_t _motorMovementEndTime = 0;

    // Helper method for timed motor operations
    static void update_timed_motor_movement();

    bool _distanceMovementInProgress = false;
    float _targetDistanceIn = 0.0f;
    float _startingDistanceIn = 0.0f;

    // Helper method for distance-based motor operations
    static void update_distance_movement();

    static void reset_state_variables(bool is_full_reset = false);

    static void pause_program();
    static void resume_program();

    void increment_pc() {
        _pc++;
    };

    bool _programContainsMotors = false;
    bool _stoppedDueToUsbSafety = false;
    bool _lastUsbState = false;

    // Sensor Activation System
    enum SensorType {
        SENSOR_QUATERNION,
        SENSOR_ACCELEROMETER,
        SENSOR_GYROSCOPE,
        SENSOR_MAGNETOMETER,
        SENSOR_TOF,
        SENSOR_SIDE_TOF,
        SENSOR_COLOR_SENSOR
    };

    static void activate_sensors_for_program();
    static const std::map<BytecodeOpCode, std::vector<SensorType>> opcodeToSensors;

    // USB Safety Methods
    static void scan_program_for_motors();
    void check_usb_safety_conditions();
    static void handle_usb_connect();

    // Add these with your other distance movement variables
    int16_t _initialDistancePwm = 0; // Store initial PWM for deceleration calculation

    // Deceleration constants (add near your other constants)
    static constexpr float DECELERATION_RATE = 2000.0f;  // PWM²/inch - tune this value
    static constexpr int16_t MIN_DECELERATION_PWM = 600; // Minimum PWM during deceleration

    // Thread synchronization
    SemaphoreHandle_t _programMutex = nullptr;
};
