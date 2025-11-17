# Demos Subsystem

High-level robot behaviors and autonomous control systems for educational demonstrations.

## Demo Manager (`demo_manager.h/cpp`)

### Core Functionality
- **Centralized demo control** via singleton pattern
- **Automatic demo switching** stops current before starting new
- **Thread-safe operations** with FreeRTOS integration
- **Status reporting** for UI and network communication
- **Demo coordination** prevents conflicts between behaviors

### Available Demos
```cpp
enum DemoType {
    NONE,                 // Idle state - manual control
    BALANCE_CONTROLLER,   // Self-balancing robot
    OBSTACLE_AVOIDER      // Navigation with collision avoidance
}
```

### Demo Lifecycle
1. **Stop current demo** if any is running
2. **Initialize new demo** with sensor requirements
3. **Enable continuous updates** via demo manager task
4. **Monitor for stop conditions** (battery, commands, errors)
5. **Clean shutdown** returns robot to safe state

## Balance Controller (`balance_controller.h/cpp`)

### Self-Balancing Algorithm
- **PID control system** with tunable parameters
- **IMU sensor fusion** using quaternion data from BNO08x
- **Motor feedback** via encoder RPM measurements
- **Angle stabilization** around configurable target angle
- **Safety limits** prevent dangerous tilting

### PID Parameters
```cpp
struct NewBalancePids {
    float pValue;                    // Proportional gain
    float iValue;                    // Integral gain
    float dValue;                    // Derivative gain
    float ffValue;                   // Feed-forward term
    float targetAngle;               // Desired balance point
    float maxSafeAngleDeviation;     // Safety shutdown limit
    float updateInterval;            // Control loop timing
    float deadbandAngle;             // Ignore small deviations
    float maxStableRotation;         // Rotation speed limit
    float minEffectivePwm;          // Minimum motor power
};
```

### Control Loop
1. **Read IMU data** for current robot orientation
2. **Calculate error** between target and actual angle
3. **PID computation** determines required motor correction
4. **Safety checks** ensure angles within safe limits
5. **Motor commands** applied with speed ramping
6. **Feedback monitoring** via wheel encoders

### Safety Features
- **Tilt protection**: Shuts down if robot tips too far
- **Battery monitoring**: Reduces performance on low battery
- **Motor limits**: Prevents excessive motor speeds
- **Timeout handling**: Stops on communication loss

## Obstacle Avoider (`obstacle_avoider.h/cpp`)

### Navigation System
- **Multi-sensor fusion**: ToF sensors
- **Dynamic path planning** around detected obstacles
- **Speed adaptation** based on obstacle proximity
- **Turning decisions** using sensor priority system
- **Recovery behaviors** when trapped or stuck

### Sensor Integration
- **Side ToF sensors**: Left/right proximity detection
- **Multizone ToF**: 64-zone forward obstacle mapping
- **Sensor fusion**: Weighted decision making from multiple inputs

### Behavior States
1. **Forward movement**: Default behavior when path is clear
2. **Obstacle detection**: Sensor monitoring and threat assessment  
3. **Avoidance maneuver**: Turning or backing away from obstacles
4. **Path replanning**: Finding alternative routes
5. **Recovery mode**: Escaping from trapped situations

### Navigation Logic
```cpp
// Simplified decision flow:
if (front_obstacle_close) {
    if (left_clear) turn_left();
    else if (right_clear) turn_right();
    else back_up_and_turn();
} else if (side_obstacle_detected) {
    adjust_heading();
} else {
    move_forward();
}
```

## Supporting Systems

### Turning Manager (`turning_manager.h/cpp`)
- **Coordinated turns** using both wheel encoders
- **Angle-based turning** with IMU feedback
- **Speed profiling** for smooth turn execution
- **Turn completion detection** via sensor fusion

### Straight Line Drive (`straight_line_drive.h/cpp`)
- **Encoder-based distance** measurement and control
- **Heading correction** using IMU for straight movement
- **Speed ramping** for smooth acceleration/deceleration
- **Obstacle integration** with real-time stopping

## Task System Integration

### Demo Manager Task
- **6KB stack allocation** for demo coordination
- **Core 1 execution** on application core
- **Medium priority** below critical system tasks
- **20ms update cycle** for responsive control

### Sensor Coordination
- **Non-blocking sensor reads** via shared data buffers
- **Rate limiting** prevents I2C bus conflicts
- **Sensor priority** during conflicting demo requirements
- **Fallback behaviors** when sensors unavailable

## Usage Examples

### Starting Demos
```cpp
DemoManager& dm = DemoManager::getInstance();
dm.startDemo(demo::DemoType::BALANCE_CONTROLLER);
dm.startDemo(demo::DemoType::OBSTACLE_AVOIDER);
dm.stopCurrentDemo(); // Return to manual control
```

### Demo Status
```cpp
if (dm.isAnyDemoActive()) {
    demo::DemoType current = dm.getCurrentDemo();
    // Adjust UI or behavior accordingly
}
```

## Configuration & Tuning

### Balance Controller Tuning
- **PID gains**: Adjust for robot weight/geometry
- **Target angle**: Calibrate for level surface
- **Safety limits**: Set conservative tilt angles
- **Update rate**: Balance responsiveness vs stability

### Obstacle Avoider Settings
- **Detection thresholds**: Distance limits for each sensor
- **Turn angles**: Avoidance maneuver parameters
- **Speed profiles**: Adaptive speed based on proximity
- **Recovery timeouts**: Maximum time in stuck states

## Troubleshooting

### Balance Controller Issues
- **Oscillation**: Reduce PID gains, especially D term
- **Slow response**: Increase P gain or reduce deadband
- **Motor saturation**: Check battery voltage and motor limits
- **IMU drift**: Recalibrate sensor or check mounting

### Obstacle Avoider Problems
- **False positives**: Adjust sensor thresholds
- **Stuck in corners**: Improve recovery algorithms
- **Erratic movement**: Check sensor noise filtering
- **Poor turning**: Verify encoder calibration

## Educational Value
- **Control theory**: Hands-on PID tuning experience
- **Sensor fusion**: Multi-modal data integration
- **Robotics concepts**: Autonomous navigation principles
- **Programming skills**: State machines and real-time control