# Utils Subsystem

Core utilities and infrastructure supporting all robot subsystems with configuration, task management, and system services.

## Configuration System

### Config (`config.h`)
- **Hardware definitions**: Pin mappings, I2C addresses, timing constants
- **Environment-aware functions**: Dynamic URL selection based on build target
- **SSL certificate storage**: Embedded root CA for secure communications
- **Compile-time constants**: `constexpr` values for optimal performance

#### Key Configuration Areas
```cpp
// I2C System
I2C_SDA_1 = 18, I2C_SCL_1 = 8;        // Primary bus
I2C_SDA_2 = 9, I2C_SCL_2 = 10;        // Battery monitor bus
I2C_CLOCK_SPEED = 100000;               // 100kHz for IMU stability

// Motor Configuration  
LEFT_MOTOR: IN1=40, IN2=39, ENC_A=47, ENC_B=48
RIGHT_MOTOR: IN1=42, IN2=41, ENC_A=2, ENC_B=1
MAX_MOTOR_PWM = 4095;

// Display & UI
SCREEN_WIDTH = 128, SCREEN_HEIGHT = 64;
LEFT_BUTTON_PIN = 11, RIGHT_BUTTON_PIN = 12;
```

### Environment Management
```cpp
// Dynamic endpoint selection
const char* getWsServerUrl() {
    // Returns local/staging/production URLs based on build
    if (env == "local") return "ws://10.76.59.40:8080/esp32"; // Changes with my local IP
    if (env == "staging") return "wss://staging-api.leverlab.com/esp32";
    return "wss://production-api.leverlabs.com/esp32";
}
```

## Task Management System

### Task Manager (`task_manager.h/cpp`)
- **Centralized task creation**: All FreeRTOS tasks managed here
- **15+ specialized tasks**: Each subsystem gets dedicated task(s)
- **Core assignment**: Strategic allocation between dual cores
- **Priority system**: 5-level priority hierarchy (0-4)
- **Stack size optimization**: Individually tuned per task

#### Core Assignment Strategy
```cpp
enum class Core : BaseType_t {
    CORE_0 = 0,    // Hardware/Real-time: sensors, motors, LEDs
    CORE_1 = 1     // Application/Communication: networking, display, demos  
};
```

#### Priority Hierarchy
```cpp
enum class Priority : uint8_t {
    BACKGROUND = 0,     // LED animations, stack monitor
    SYSTEM_CONTROL = 1, // Sensor polling, bytecode VM
    COMMUNICATION = 2,  // Serial input, network management  
    REALTIME_COMM = 3,  // WebSocket (low latency required)
    CRITICAL = 4        // Buttons, serial queue (immediate response)
};
```

#### Stack Size Allocations
```cpp
// Carefully tuned per task requirements
BUTTON_STACK_SIZE = 4096;              // Simple input handling
BYTECODE_VM_STACK_SIZE = 16384;        // Largest - user program execution
MULTIZONE_TOF_STACK_SIZE = 8192;       // Heavy processing, 64 zones
NETWORK_MANAGEMENT_STACK_SIZE = 8192;   // WiFi operations
SERIAL_QUEUE_STACK_SIZE = 10240;       // Large message buffers
```

## Data Structures (`structs.h`)

### Sensor Data Types
```cpp
// IMU sensor fusion
struct QuaternionData { float qX, qY, qZ, qW; bool isValid; };
struct EulerAngles { float yaw, pitch, roll; bool isValid; };

// Battery comprehensive monitoring
struct BatteryState {
    unsigned int realStateOfCharge, voltage;
    int current, power;
    float estimatedTimeToEmpty;
    bool isCharging, isLowBattery, isCriticalBattery;
};

// LED control system
struct LedState {
    uint8_t colors[8][3];              // RGB for all 8 LEDs
    led_types::AnimationType animation;  // Breathing, strobing, rainbow
    int animationSpeed;                // 1-10 scale
};
```

### Communication Protocols  
```cpp
// Network data structures
struct WiFiCredentials { String ssid, password; };
struct WiFiNetworkInfo { String ssid; int32_t rssi; uint8_t encryptionType; };

// Message routing system
enum class RouteType {
    REGISTER, SENSOR_DATA, BYTECODE_STATUS,
    WIFI_CONNECTION_RESULT, BATTERY_MONITOR_DATA_FULL
    // ... 15+ route types for comprehensive communication
};
```

## System Services

### Singleton Pattern (`singleton.h`)
- **Thread-safe singleton**: Template-based implementation
- **Lazy initialization**: Created on first access
- **No dynamic allocation**: Stack-based singleton storage
- **Used by all managers**: Consistent access pattern across codebase

```cpp
template<typename T>
class Singleton {
public:
    static T& get_instance() {
        static T instance;
        return instance;
    }
protected:
    Singleton() = default;
};
```

### Preferences Manager (`preferences_manager.h/cpp`)
- **Persistent storage**: WiFi credentials, settings, calibration data
- **NVS integration**: ESP32 non-volatile storage API
- **Namespace organization**: Logical grouping of related settings
- **Type safety**: Template-based get/set operations

### Hold-to-Wake System (`hold_to_wake.h/cpp`)
- **Power management**: Deep sleep entry/exit coordination
- **Button monitoring**: 1-second hold detection for wake
- **Display integration**: Screen initialization after wake
- **Battery protection**: Prevents wake on critical battery

## Utility Functions

### General Utilities (`utils.h/cpp`)
- **Math helpers**: Constrain, map, interpolation functions
- **String utilities**: Parsing and formatting helpers  
- **Time management**: Millisecond timing and delays
- **Debug helpers**: Memory usage, system info reporting

### System Information (`show_chip_info.h/cpp`)
- **Hardware identification**: Chip model, revision, features
- **Memory reporting**: Flash, PSRAM, heap usage
- **Performance metrics**: CPU frequency, core count
- **Debug output**: Formatted system information display

### Timeout Management (`timeout_manager.h/cpp`)
- **Configurable timeouts**: Per-operation timeout settings
- **Non-blocking checks**: Async timeout monitoring
- **Resource cleanup**: Automatic cleanup on timeout
- **System integration**: Coordinates with task system

## Logging & Monitoring

### Sensor Loggers (`sensor_loggers.h/cpp`)
- **Real-time logging**: Sensor data streaming to serial
- **Selective logging**: Enable/disable per sensor type
- **Performance monitoring**: Update rates and timing analysis
- **Debug formatting**: Human-readable sensor output

### System Monitoring
- **Stack usage tracking**: Per-task stack monitoring
- **Heap monitoring**: Dynamic memory usage tracking
- **Task statistics**: Runtime analysis and performance metrics
- **Error reporting**: System health and failure detection

## Development Support

### Debug Configuration
```cpp
// Environment-based debug levels
#if defined(LOCAL_ENVIRONMENT)
    #define CORE_DEBUG_LEVEL 5    // Verbose debugging
#elif defined(STAGING_ENVIRONMENT)  
    #define CORE_DEBUG_LEVEL 3    // Warning level
#else
    #define CORE_DEBUG_LEVEL 1    // Error only
#endif
```

### Memory Management
- **Static allocation preferred**: Minimize dynamic allocation
- **Stack monitoring**: Prevent overflow conditions
- **Resource tracking**: Monitor usage patterns
- **Optimization guidelines**: Memory-efficient coding practices

## Integration Patterns

### Cross-Subsystem Communication
- **Shared data structures**: Common types in `structs.h`
- **Singleton access**: Consistent manager access pattern
- **Event coordination**: Task synchronization mechanisms
- **Error propagation**: Standardized error handling

### Initialization Sequence
1. **Power management**: Enable system power rails
2. **Serial communication**: Early debug output capability
3. **Task creation**: Core services and sensor tasks
4. **Hardware initialization**: Sensors, actuators, communication
5. **Application startup**: Demos, user programs, network

## Performance Considerations

### Real-Time Requirements
- **Task priorities**: Ensure real-time task scheduling
- **Stack sizing**: Prevent stack overflow while minimizing RAM
- **CPU allocation**: Balance between cores for optimal performance
- **Interrupt handling**: Minimize interrupt latency

### Memory Optimization
- **Compile-time configuration**: `constexpr` for constants
- **Structured data**: Efficient packing and alignment
- **Resource pooling**: Reuse allocated resources
- **Garbage collection**: Automatic cleanup patterns

## Troubleshooting

### System Issues
- **Task creation failures**: Check available memory and stack sizes
- **Priority inversions**: Review task priority assignments
- **Watchdog resets**: Monitor task timing and blocking operations
- **Memory corruption**: Use stack monitoring and bounds checking

### Performance Problems
- **High CPU usage**: Profile task execution times
- **Memory leaks**: Monitor heap usage over time
- **I2C conflicts**: Coordinate sensor access timing
- **Power issues**: Check current consumption patterns

## Maintenance Guidelines
- **Regular stack monitoring**: Verify adequate stack margins
- **Performance profiling**: Monitor system timing characteristics  
- **Configuration updates**: Keep pin mappings and constants current
- **Code reviews**: Ensure consistent utility usage patterns