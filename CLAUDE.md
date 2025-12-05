# Educational Robot Firmware - ESP32-S3

This is an embedded systems project using PlatformIO with Arduino Framework for an educational robot with dual-core ESP32-S3 MCU.

## Hardware Configuration

### MCU & Core Usage
- **MCU**: ESP32-S3 (240MHz, dual-core)
- **Core 0**: Hardware/real-time tasks (sensors, motors)
- **Core 1**: Communication/application tasks (networking, display)
- **Memory**: 16MB flash (8MB PSRAM) with custom partitions for OTA updates

### I2C Configuration
**Primary I2C Bus** (SDA: 18, SCL: 8, 100kHz):
- Left Side ToF: 0x51
- Right Side ToF: 0x60  
- Multizone ToF: 0x29
- IMU (BNO08x): 0x4A
- Display (SSD1306): 0x3C
- Color Sensor (VEML3328): 0x10

**Secondary I2C Bus** (SDA: 9, SCL: 10):
- Battery Monitor (BQ27441): 0x55

### Pin Mappings
```cpp
// Motors & Encoders
LEFT_MOTOR: IN1=40, IN2=39, ENC_A=47, ENC_B=48
RIGHT_MOTOR: IN1=42, IN2=41, ENC_A=2, ENC_B=1

// Audio (I2S)
SPEAKER: DOUT=13, BCLK=14, LRC=21

// LEDs & Lighting
NEOPIXEL_STRIP=4 (8 LEDs), COLOR_LED=5

// Controls
BUTTONS: LEFT=11, RIGHT=12
POWER_EN=38
```

## Architecture

### Task System (FreeRTOS)
**Core 0 - Hardware/Real-time**:
- Motor control, sensor polling, LED animations
- High-priority tasks with fast response times

**Core 1 - Communication/Application**:  
- WiFi/WebSocket, serial communication, display updates
- Demo management, bytecode interpreter

### Key Design Patterns
- **Singleton pattern**: All managers use `::get_instance()`
- **Task-based architecture**: 15+ specialized FreeRTOS tasks
- **Centralized initialization**: `SensorInitializer` prevents I2C conflicts
- **Rate limiting**: All sensors have configurable update intervals
- **Data buffering**: Thread-safe sensor data sharing

## Build System

### Environments
```bash
# Development (full debugging)
pio run -e local

# Production (minimal logging)
pio run -e production
```

### Upload & Monitor
```bash
pio run -t upload -t monitor -e local --upload-port /dev/cu.usbmodem101
```

### Key Dependencies
- ArduinoWebsockets, ArduinoJson
- Adafruit NeoPixel, BNO08x, SSD1306
- ESP32Encoder, Button2
- ESP8266Audio

## Major Subsystems

### 1. Sensor System
- **IMU**: Quaternion, Euler angles, accelerometer data
- **ToF Sensors**: Side (proximity) + multizone (64-zone mapping)
- **Color Sensor**: RGB detection with calibration matrix
- **Battery Monitor**: Voltage, current, health tracking

### 2. Demo System
- **Balance Controller**: Self-balancing with PID control
- **Obstacle Avoider**: Navigation using ToF sensors
- **Demo Manager**: Coordinates between different behaviors

### 3. Communication
- **Serial Protocol**: 30+ message types with START/END markers
- **WebSocket**: Real-time bidirectional communication
- **WiFi Management**: Auto-connect with credential storage

### 4. Custom Interpreter
- **Bytecode VM**: Executes user programs up to 8KB
- **Sandboxed execution**: Safe user code execution
- **Program management**: Upload, pause, stop functionality

### 5. Audio System
- **Tone Playback**:
- **Volume Control**: Software-controlled audio levels

## Development Guidelines

### Code Conventions
- Use `constexpr` for constants (not `#define`)
- Rate-limit all sensor readings with timeouts
- Handle I2C failures gracefully with retry logic  
- Use structured data types from `utils/structs.h`
- Follow task priority guidelines (0-4, higher = more critical)

### Memory Management
- **Stack Sizes**: Carefully tuned per task (4KB-16KB)
- **Serial Buffers**: Enlarged to 8KB for bytecode uploads
- **Partition Layout**: Custom CSV for OTA + NVS storage

### Networking
- **Environment URLs**: Local/production endpoints
- **Certificate Pinning**: Embedded root CA for HTTPS
- **Connection States**: WiFi-only vs full WebSocket connectivity

## Troubleshooting

### Common Issues
- **Build Errors**: Run "Full Clean" if linking fails
- **I2C Conflicts**: Check sensor initialization order
- **Memory**: Monitor stack usage with StackMonitor task

### Debug Features
- **Stack Monitor**: Real-time task stack usage
- **Sensor Logging**: Individual sensor debug output

## Important Notes
- **No auto-build**: After editing code, do not build the project automatically
- **Deep Sleep**: Hold-to-wake functionality on button press
- **Battery Safety**: Automatic shutdown on critical battery levels
- **OTA Updates**: Dual partition system for safe firmware updates

## Environment Variables
Each build environment has unique:
- `DEFAULT_PIP_ID`: Robot identification
- `CORE_DEBUG_LEVEL`: Logging verbosity
- API endpoints for production deployment
