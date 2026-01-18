# Pip-Bot Firmware

Educational robot firmware for ESP32-S3 featuring dual-core real-time control, multi-sensor fusion, and a custom bytecode interpreter for user programs.

## Features

- **Dual-Core Architecture**: Core 0 handles real-time hardware (sensors, motors), Core 1 manages communication and application logic
- **10+ Integrated Sensors**: IMU, 3x Time-of-Flight, color sensor, wheel encoders, battery monitor
- **Autonomous Behaviors**: Self-balancing with PID control, obstacle avoidance with path planning
- **Custom Bytecode VM**: Safe sandboxed execution of user programs up to 8KB
- **Wireless Control**: WiFi + WebSocket for real-time bidirectional communication
- **Educational Games**: Built-in games and curriculum-aligned triggers

## Hardware

| Component | Details |
|-----------|---------|
| MCU | ESP32-S3 (240MHz dual-core) |
| Memory | 16MB Flash, 8MB PSRAM |
| IMU | BNO08x (quaternion, euler, accel/gyro) |
| ToF Sensors | VL53L7CX (64-zone), 2x VCNL36828P (side) |
| Color Sensor | VEML3328 (RGB with calibration) |
| Display | SSD1306 128x64 OLED |
| Audio | I2S speaker output |
| LEDs | 8x NeoPixel strip + RGB headlight |
| Motors | Dual H-bridge with quadrature encoders |
| Battery | BQ27441 fuel gauge monitoring |

### Pin Configuration

```
Motors:     L(IN1=40, IN2=39), R(IN1=42, IN2=41)
Encoders:   L(A=47, B=48), R(A=2, B=1)
I2C Primary: SDA=18, SCL=8
I2C Battery: SDA=9, SCL=10
Audio:      DOUT=13, BCLK=14, LRC=21
NeoPixels:  Pin 4 (8 LEDs)
Buttons:    Left=11, Right=12
```

## Getting Started

### Prerequisites

- [VS Code](https://code.visualstudio.com/)
- [PlatformIO Extension](https://platformio.org/install/ide?install=vscode)

### Build & Upload

```bash
# Development build (full debugging)
pio run -e local

# Production build (minimal logging)
pio run -e production

# Upload and monitor
pio run -t upload -t monitor -e local --upload-port /dev/cu.usbmodem101
```

### Configuration

To assign a unique robot ID, modify `DEFAULT_PIP_ID` in `src/utils/pip_config.h`.

## Architecture

```
src/
├── main.cpp              # Entry point, task initialization
├── actuators/            # Motors, LEDs, display, audio
├── sensors/              # IMU, ToF, color, battery, encoders
├── networking/           # WiFi, WebSocket, serial protocol
├── demos/                # Balance controller, obstacle avoider
├── games/                # Dino runner game
├── custom_interpreter/   # Bytecode VM for user programs
└── utils/                # Config, task manager, data structures
```

### FreeRTOS Task Distribution

**Core 0 (Hardware/Real-time)**:
- Motor control, sensor polling, LED animations

**Core 1 (Communication/Application)**:
- WiFi/WebSocket, serial protocol, display, demos

### Key Design Patterns

- **Singleton Pattern**: All managers use `::get_instance()`
- **Centralized Initialization**: `SensorInitializer` prevents I2C conflicts
- **Thread-Safe Data Sharing**: `SensorDataBuffer` for inter-task communication
- **Rate Limiting**: Configurable update intervals for all sensors

## Communication

### Serial Protocol

- 115,200 baud over USB CDC
- START/END markers (0xAA/0x55)
- 30+ message types for motor control, LED animations, sensor data, bytecode upload

### WebSocket

- Production: `wss://production-api.leverlabs.com/esp32`
- Local: `ws://10.213.255.40:8080/esp32`
- TLS with embedded root CA certificate

## Autonomous Behaviors

### Self-Balancing

PID-controlled balance using IMU quaternion data with tunable gains, safety tilt limits, and encoder feedback.

### Obstacle Avoidance

Multi-sensor fusion combining side ToF proximity and 64-zone multizone mapping for dynamic path planning and recovery behaviors.

## Safety Features

- Motor auto-stop after 1 second without commands
- Critical battery shutdown with charging detection
- Deep sleep with hold-to-wake power management
- Balance controller tilt limits
- LED brightness limiting (max 75)

## Troubleshooting

**Build/link errors**: Run PlatformIO "Full Clean"

**I2C issues**: Check sensor initialization order in `SensorInitializer`

**Stack overflow**: Monitor with `StackMonitor` task (enable in config)

## License

MIT License - see [LICENSE](LICENSE) for details.
