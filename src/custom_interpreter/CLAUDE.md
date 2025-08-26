# Custom Interpreter Subsystem

A sandboxed bytecode virtual machine for executing user programs safely on the ESP32.

## Architecture Overview

### Bytecode VM (`bytecode_vm.h/cpp`)
- **Stack-based virtual machine** with custom instruction set
- **8KB program limit** with enlarged serial buffers for upload
- **Sandboxed execution** prevents system interference
- **Real-time control** of robot subsystems
- **Safe termination** with cleanup on program end

### Bytecode Structures (`bytecode_structs.h`)
- **Instruction format** definitions for VM opcodes
- **Data type representations** for variables and constants
- **Stack frame layouts** for function calls
- **Memory organization** for program and data segments

## VM Capabilities

### Instruction Set
- **Arithmetic operations**: Add, subtract, multiply, divide
- **Logic operations**: AND, OR, NOT, comparison operators
- **Control flow**: Jumps, conditional branches, loops
- **Stack manipulation**: Push, pop, duplicate, swap
- **System calls**: Robot control functions

### Robot Integration
- **Motor control**: Direct speed and direction commands
- **Sensor access**: Read IMU, ToF, color, and IR sensors
- **LED control**: Individual LED color and animation control
- **Audio commands**: Play sounds and control volume
- **Timing functions**: Delays and scheduling

### Safety Features
- **Memory isolation**: Programs cannot access system memory
- **Execution limits**: Prevents infinite loops and resource exhaustion
- **Controlled I/O**: Only approved robot functions accessible
- **Emergency stop**: Can be terminated from external commands
- **Resource cleanup**: Automatic cleanup on program termination

## Program Lifecycle

### Upload Process
1. **Serial reception**: Large programs uploaded via enlarged buffers
2. **Bytecode validation**: Syntax and safety checks before execution
3. **Memory allocation**: Secure program space allocation
4. **VM initialization**: Stack and register setup

### Execution Flow
1. **Program counter**: Tracks current instruction
2. **Stack management**: Local variables and function calls
3. **System calls**: Controlled access to robot functions
4. **State updates**: Real-time robot control during execution

### Termination
1. **Clean shutdown**: Proper resource deallocation
2. **Robot reset**: Return actuators to safe states
3. **Memory cleanup**: Free allocated program space
4. **Status reporting**: Execution results to host system

## Task Integration

### VM Task
- **Dedicated FreeRTOS task** (16KB stack - largest allocation)
- **Core 1 assignment**: Runs on application core
- **Medium priority**: Below critical system tasks
- **Cooperative scheduling**: Yields control periodically

### Communication
- **Serial protocol integration**: Uses message processor
- **Status reporting**: Execution state updates
- **Program management**: Upload, start, pause, stop commands
- **Error handling**: Safe failure modes with system recovery

## Security Model

### Sandboxing
- **Memory protection**: Cannot access outside allocated space
- **System isolation**: No direct hardware register access
- **Function whitelisting**: Only approved robot operations
- **Resource limiting**: CPU time and memory usage bounds

### Safe Operations
- **Motor limits**: Speed and acceleration constraints
- **Sensor polling**: Rate-limited to prevent I2C conflicts
- **Audio controls**: Volume and duration limits
- **LED safety**: Brightness and pattern restrictions

## Usage Patterns

### Program Structure
```
// Typical user program flow:
1. Sensor reading
2. Decision logic
3. Robot actions (motors, LEDs, audio)
4. Timing/delays
5. Loop or termination
```

### System Integration
- **Demo coordination**: Can be controlled by demo manager
- **Network commands**: Remote program upload and control
- **Sensor sharing**: Coordinated access with system sensors
- **Actuator arbitration**: Managed access to motors and LEDs

## Development Notes

### Memory Considerations
- **Program size**: 8KB maximum user program
- **Stack depth**: Limited function call nesting
- **Variable storage**: Efficient bytecode encoding
- **Garbage collection**: Automatic memory management

### Performance
- **Execution speed**: Optimized for real-time robot control
- **Responsiveness**: Maintains system task scheduling
- **Power efficiency**: Minimal idle power consumption
- **Reliability**: Handles malformed programs gracefully

### Runtime Problems
- **Stack overflow**: Monitor program complexity and nesting
- **Infinite loops**: Implementation has execution time limits
- **Resource conflicts**: Coordinate with other system tasks
- **Memory leaks**: VM handles cleanup automatically
