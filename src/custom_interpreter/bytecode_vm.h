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
    bool load_program(const uint8_t* byteCode, uint16_t size);
    void stop_program();

    // Debug methods for distance movement
    bool is_distance_movement_active() const {
        return distanceMovementInProgress;
    }
    float get_target_distance_in() const {
        return targetDistanceIn;
    }
    float get_starting_distance_in() const {
        return startingDistanceIn;
    }
    bool is_program_loaded() const {
        return program != nullptr;
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

    BytecodeInstruction* program = nullptr;
    uint16_t programSize = 0;
    uint16_t pc = 0;         // Program counter
    uint32_t delayUntil = 0; // For handling delays
    bool waitingForDelay = false;
    bool lastComparisonResult = false; // Stores result of last comparison

    // Union to store different variable types in the same memory
    union RegisterValue {
        float asFloat;
        int32_t asInt;
        bool asBool;
        uint8_t asBytes[4];
    };

    RegisterValue registers[MAX_REGISTERS];
    BytecodeVarType registerTypes[MAX_REGISTERS];
    bool registerInitialized[MAX_REGISTERS] = {false};

    // Update VM - call this regularly from main loop
    void update();

    enum PauseState { PROGRAM_NOT_STARTED, PAUSED, RUNNING, PROGRAM_FINISHED };

    PauseState isPaused = PauseState::PROGRAM_NOT_STARTED;

    bool waitingForButtonPressToStart = false;
    bool can_start_program();

    // Execute instruction implementation
    void execute_instruction(const BytecodeInstruction& instr);

    // Helper method for comparisons
    bool compare_values(ComparisonOp op, float leftOperand, float rightValue);

    bool timedMotorMovementInProgress = false;
    uint32_t motorMovementEndTime = 0;

    // Helper method for timed motor operations
    void update_timed_motor_movement();

    bool distanceMovementInProgress = false;
    float targetDistanceIn = 0.0f;
    float startingDistanceIn = 0.0f;

    // Helper method for distance-based motor operations
    void update_distance_movement();

    void reset_state_variables(bool isFullReset = false);

    void pause_program();
    void resume_program();

    void increment_pc() {
        pc++;
    };

    bool programContainsMotors = false;
    bool stoppedDueToUsbSafety = false;
    bool lastUsbState = false;

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

    void activate_sensors_for_program();
    static const std::map<BytecodeOpCode, std::vector<SensorType>> opcodeToSensors;

    // USB Safety Methods
    void scan_program_for_motors();
    void check_usb_safety_conditions();
    void handle_usb_connect();

    // Add these with your other distance movement variables
    int16_t initialDistancePwm = 0; // Store initial PWM for deceleration calculation

    // Deceleration constants (add near your other constants)
    static constexpr float DECELERATION_RATE = 2000.0f;  // PWM²/inch - tune this value
    static constexpr int16_t MIN_DECELERATION_PWM = 600; // Minimum PWM during deceleration

    // Thread synchronization
    SemaphoreHandle_t programMutex = nullptr;
};
