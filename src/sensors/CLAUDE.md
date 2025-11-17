# Sensors Subsystem

Comprehensive sensor management system providing real-time environmental and state monitoring for robot control.

## Architecture Overview

### Centralized Initialization (`sensor_initializer.h/cpp`)
- **Single I2C coordinator**: Prevents bus conflicts during startup
- **Sequential sensor setup**: Ordered initialization for reliability
- **Error handling**: Graceful failure modes for missing sensors
- **Status reporting**: Initialization success/failure logging

### Sensor Data Management (`sensor_data_buffer.h/cpp`)
- **Thread-safe data sharing**: Producer/consumer pattern between tasks
- **Timeout handling**: Prevents stale data usage (5s timeout)
- **Memory efficient**: Structured data types minimize RAM usage
- **Real-time updates**: Low-latency sensor data access

## Individual Sensors

### IMU - Inertial Measurement Unit (`imu.h/cpp`)
- **Sensor**: Adafruit BNO08x (I2C 0x4A)
- **Data types**: Quaternion, Euler angles, accelerometer, gyroscope
- **Sensor fusion**: Hardware-based orientation calculation
- **Update rate**: 100Hz for real-time balance control
- **Calibration**: Automatic calibration with status reporting

#### Data Structures
```cpp
struct QuaternionData { float qX, qY, qZ, qW; bool isValid; };
struct EulerAngles { float yaw, pitch, roll; bool isValid; };
struct AccelerometerData { float aX, aY, aZ; bool isValid; };
```

### Time-of-Flight Sensors

#### Side ToF (`side_time_of_flight_sensor.h/cpp`, `side_tof_manager.h/cpp`)
- **Sensors**: VCNL36828P proximity sensors (I2C 0x51, 0x60)
- **Range**: Short-range obstacle detection (left/right sides)
- **Update rate**: 50Hz for collision avoidance
- **Integration**: Obstacle avoider and navigation demos

#### Multizone ToF (`multizone_tof_sensor.h/cpp`)
- **Sensor**: VL53L7CX (I2C 0x29)
- **Capability**: 64-zone distance mapping (8x8 grid)
- **Range**: Up to 4 meters with high accuracy
- **Processing**: Zone analysis for object detection and mapping
- **Applications**: Advanced navigation and room mapping

### Color Sensor (`color_sensor.h/cpp`)
- **Sensor**: VEML3328 (I2C 0x10)
- **RGB detection**: Black/white point calibrated color recognition
- **Illumination**: Downward LED (pin 5) with variable brightness (0-255)
- **Power management**: Automatic enable/disable based on usage timeouts
- **Calibration system**: Black and white point normalization for accurate readings
- **Applications**: Surface color detection, line following

#### Black/White Point Calibration System
```cpp
struct CalibrationValues {
    uint16_t blackRed, blackGreen, blackBlue;    // Dark reference values
    uint16_t whiteRed, whiteGreen, whiteBlue;    // Bright reference values
};
```

#### Calibration Features
- **Two-point calibration**: Black and white reference points for linear normalization
- **Averaged readings**: 5 samples per calibration point for stability
- **Safe normalization**: Constrained 0-255 output with overflow protection
- **Fallback mode**: Simple 8-bit conversion when uncalibrated

### Wheel Encoders (`encoder_manager.h/cpp`)
- **Sensors**: Quadrature encoders on both motors
- **Pin mapping**: Left (47,48), Right (2,1)
- **Resolution**: 3 CPR with 297.924:1 gear ratio
- **Measurements**: RPM, distance, wheel slip detection
- **Integration**: Motor control feedback and odometry

#### Encoder Calculations
```cpp
static constexpr float GEAR_RATIO = 297.924;
static constexpr uint8_t ENCODER_CPR = 3;
static constexpr float WHEEL_DIAMETER_CM = 3.9;
static constexpr uint32_t RPM_CALC_INTERVAL = 20; // ms
```

### Battery Monitor (`battery_monitor.h/cpp`)
- **Sensor**: SparkFun BQ27441 (I2C 0x55, dedicated I2C bus)
- **Dedicated I2C**: Secondary bus (SDA=9, SCL=10) prevents conflicts
- **Comprehensive monitoring**: Voltage, current, capacity, health
- **Safety features**: Critical battery shutdown, charging detection
- **Power management**: Deep sleep coordination

#### Battery Data
```cpp
struct BatteryState {
    uint32_t realStateOfCharge;    // 0-100%
    uint32_t voltage;              // mV
    int current;                       // mA (+ discharge, - charge)
    int power;                         // mW
    float estimatedTimeToEmpty;        // hours
    bool isCharging, isDischarging;
    bool isLowBattery, isCriticalBattery;
};
```

## Task System Integration

### Individual Sensor Tasks
Each sensor runs in its own FreeRTOS task with optimized parameters:

```cpp
// Task allocations (Core 0 - Real-time)
IMU_SENSOR_STACK_SIZE = 4096;           // Fast, lightweight
ENCODER_SENSOR_STACK_SIZE = 4096;       // Fast, lightweight  
MULTIZONE_TOF_STACK_SIZE = 8192;        // Heavy processing, 64 zones
SIDE_TOF_STACK_SIZE = 6144;             // Moderate processing
COLOR_SENSOR_STACK_SIZE = 4096;         // Light processing
BATTERY_MONITOR_STACK_SIZE = 6144;      // I2C + calculations
```

### Sensor Coordination
- **Rate limiting**: Prevents I2C bus saturation
- **Priority handling**: Critical sensors get bus priority
- **Error recovery**: Automatic sensor re-initialization
- **Data validation**: Range checking and sanity verification

## Sensor Data Flow

### Data Pipeline
1. **Hardware polling**: Individual sensor tasks read hardware
2. **Data validation**: Range checks and error detection
3. **Buffer storage**: Thread-safe data structures
4. **Consumer access**: Other systems read validated data
5. **Timeout management**: Stale data detection and handling

### Integration Points
- **Demo system**: Sensor fusion for autonomous behaviors
- **Network communication**: Real-time sensor data streaming
- **Safety systems**: Battery monitoring and obstacle detection
- **User programs**: Sensor access via bytecode VM

## Calibration & Configuration

### Sensor Calibration
- **IMU calibration**: Automatic gyroscope and magnetometer cal
- **Color sensor**: Black/white point calibration with averaged readings
- **Encoder calibration**: Wheel diameter and gear ratio tuning
- **ToF sensors**: Distance offset and noise filtering

### Performance Tuning
- **Update rates**: Configurable per sensor type
- **Filtering**: Software noise reduction and smoothing
- **Thresholds**: Adjustable detection limits per application
- **Power management**: Sensor enable/disable for battery life

## Usage Examples

### Reading Sensor Data
```cpp
// IMU data access
QuaternionData quat = SensorDataBuffer::get_instance().getQuaternionData();
if (quat.isValid) {
    // Use quaternion for balance control
}

// Battery monitoring
BatteryState battery = BatteryMonitor::get_instance().getBatteryState();
if (battery.isCriticalBattery) {
    // Initiate emergency shutdown
}
```

### Sensor Status Checking
```cpp
// Verify sensor initialization
if (ColorSensor::get_instance().isInitialized()) {
    ColorData colorData = SensorDataBuffer::get_instance().getColorData();
    if (colorData.isValid) {
        // Process RGB values: colorData.redValue, greenValue, blueValue
    }
}
```

### Color Sensor Calibration
```cpp
// Perform black point calibration (on dark surface)
ColorSensor::get_instance().calibrateBlackPoint();

// Perform white point calibration (on white surface)  
ColorSensor::get_instance().calibrateWhitePoint();
```

## Troubleshooting

### I2C Bus Issues
- **Bus lockup**: Check pull-up resistors and cable integrity
- **Address conflicts**: Verify each sensor has unique address
- **Timing issues**: Reduce I2C clock speed if communication fails
- **Power problems**: Ensure stable 3.3V supply to all sensors

### Sensor-Specific Problems

#### IMU Issues
- **Calibration problems**: Ensure robot is stationary during startup
- **Drift**: Check mounting and vibration isolation
- **Data validity**: Monitor isValid flags for communication errors

#### ToF Sensor Problems
- **Range issues**: Check for IR interference and reflective surfaces
- **Update lag**: Verify sensor configuration and I2C timing
- **False readings**: Implement median filtering for noise reduction

#### Battery Monitor
- **Incorrect readings**: Verify I2C connection on secondary bus
- **Charging detection**: Check power supply and charging circuit
- **Capacity errors**: May require BQ27441 learning cycle

### Performance Issues
- **High CPU usage**: Reduce sensor update rates if necessary
- **Memory consumption**: Monitor stack usage per sensor task
- **I2C congestion**: Stagger sensor readings to distribute bus load
- **Power consumption**: Disable unused sensors for battery life

## Maintenance & Updates
- **Sensor libraries**: Keep manufacturer libraries updated
- **Calibration data**: Periodic recalibration for accuracy
- **Hardware inspection**: Check connections and sensor mounting
- **Performance monitoring**: Track sensor reliability and timing