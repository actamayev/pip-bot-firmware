# Actuators Subsystem

This directory contains all output devices and user interface components for the robot.

## Components Overview

### Motors (`motor_driver.h/cpp`)
- **Dual H-bridge control** for left/right wheels
- **Speed ramping** prevents sudden acceleration (50 PWM/step)
- **Timeout protection** stops motors after 1s without commands
- **Encoder integration** for closed-loop control
- **Pin mapping**: L(40,39), R(42,41) + encoders

### LEDs (`led/`)
- **NeoPixel strip** (8 LEDs on pin 4)
- **Animations**: Breathing, strobing, rainbow, fade-out
- **Individual control** per LED with RGB values
- **Animation speed** configurable (1-10 scale)
- **Brightness limiting** (max 75 to prevent power issues)

### Audio (`speaker.h/cpp`)
- **I2S audio output** (DOUT=13, BCLK=14, LRC=21)
- **Volume control** (0.0-4.0 range)
- **Mute functionality** for quiet operation

### Display (`display_screen.h/cpp`)
- **128x64 OLED** (SSD1306, I2C 0x3C)
- **Multi-screen UI**: startup, battery, WiFi selection
- **Smart buffer optimization**: Only updates I2C when content changes
- **Performance tracking**: Monitors update rates and I2C efficiency
- **Real-time updates** via dedicated task
- **Low battery warnings** with automatic display

### Buttons (`buttons.h/cpp`)
- **Dual buttons** (pins 11, 12) with debouncing
- **Deep sleep control** via hold-to-wake
- **Power management** integration
- **Button2 library** for reliable input handling

## Key Design Patterns

### Task Integration
- Each actuator runs in dedicated FreeRTOS tasks
- **Core assignment**: Most actuators on Core 0 for real-time response
- **Stack sizes**: Tuned per component (4KB-12KB)
- **Priorities**: Motors highest (4), LEDs lowest (0)

### Safety Features
- **Motor timeout**: Auto-stop prevents runaway
- **Brightness limiting**: Prevents LED overcurrent
- **Audio volume caps**: Protects speakers
- **Power state awareness**: Coordinated shutdown

### Communication Patterns
- **Singleton managers**: Thread-safe access via `::get_instance()`
- **Command queuing**: Non-blocking control interfaces
- **State persistence**: Settings survive power cycles
- **Error reporting**: Status feedback to main system

## Usage Examples

### Motor Control
```cpp
MotorDriver::get_instance().set_motor_speeds(leftSpeed, rightSpeed);
MotorDriver::get_instance().stop_both_motors(); // Emergency stop
```

### LED Animation
```cpp
RgbLed& leds = rgb_led;
leds.setAnimation(led_types::BREATHING, 5); // Speed 1-10
leds.setLedColor(0, 255, 0, 0); // LED 0 to red
```

### Audio Playback
```cpp
Speaker::get_instance().setVolume(2.0); // 0.0-4.0 range
```

## Hardware Dependencies

### Power Requirements
- **Motors**: High current draw, requires power management
- **LEDs**: Limited to 75 brightness to prevent brownouts
- **Audio**: I2S requires stable power for clean output

### Timing Constraints
- **Motor updates**: 20ms intervals for smooth control
- **LED animations**: 50ms refresh for visible effects
- **Display updates**: 50ms content generation, I2C only on changes
- **Button debouncing**: 50ms standard debounce time

## Troubleshooting

### Motor Issues
- Check encoder wiring if RPM readings are incorrect
- Verify H-bridge connections for direction problems
- Monitor power supply under load

### LED Problems
- Reduce brightness if LEDs flicker or cause resets
- Check pin 4 connection for no-response issues
- Verify NeoPixel library version compatibility

### Audio Troubles
- Ensure I2S pins are correctly configured
- Monitor serial output for initialization errors

## Integration Notes
- **Display initialization**: Must occur after I2C setup
- **Motor safety**: Coordinate with battery monitor
- **LED effects**: Can be overridden by demo modes
- **Button handling**: Integrates with network connectivity