# Networking Subsystem

Comprehensive communication system handling WiFi, WebSocket, and serial protocols for robot connectivity.

## Architecture Overview

### Dual-Layer Communication
- **Local communication**: Serial USB for development and programming
- **Remote communication**: WiFi + WebSocket for wireless control
- **Protocol bridge**: Messages route between serial and network
- **Environment switching**: Local/staging/production endpoints

## Core Components

### WiFi Manager (`wifi_manager.h/cpp`)
- **Auto-connect**: Attempts saved networks in priority order
- **Credential storage**: Persistent WiFi settings via Preferences
- **Network scanning**: Both soft (cached) and hard (forced) scans
- **Connection states**: WiFi-only vs full WebSocket connectivity
- **Signal monitoring**: RSSI tracking and connection quality

### WebSocket Manager (`websocket_manager.h/cpp`)
- **Real-time communication**: Bidirectional message exchange
- **Connection resilience**: Automatic reconnection with backoff
- **Certificate validation**: Pinned root CA for secure connections
- **Environment URLs**: 
  - Staging: `wss://staging-api.leverlabs.com/esp32`
  - Production: `wss://production-api.leverlabs.com/esp32`

### Serial Communication

#### Serial Manager (`serial_manager.h/cpp`)
- **USB communication**: Primary interface for development
- **Message parsing**: START/END marker protocol (0xAA/0x55)
- **Large buffer support**: 8KB for bytecode program uploads
- **Command processing**: Real-time robot control via serial

#### Serial Queue Manager (`serial_queue_manager.h/cpp`)
- **Thread-safe messaging**: Producer/consumer pattern
- **Message buffering**: Handles burst communication
- **Priority handling**: Critical messages bypass normal queue
- **Overflow protection**: Prevents memory exhaustion

## Protocol System

### Message Processor (`message_processor.h/cpp`)
- **30+ message types**: Comprehensive command set
- **Binary protocol**: Efficient data encoding
- **Command routing**: Distributes messages to appropriate subsystems
- **Response generation**: Status feedback and sensor data

### Protocol Definition (`protocol.h`)

#### Key Message Types
```cpp
enum class DataMessageType : uint8_t {
    MOTOR_CONTROL = 1,           // Robot movement commands
    TONE_COMMAND = 2,           // Tone control
    BALANCE_CONTROL = 4,         // Self-balancing demo
    UPDATE_LED_COLORS = 7,       // Individual LED control
    BYTECODE_PROGRAM = 8,        // User program upload
    OBSTACLE_AVOIDANCE = 10,     // Navigation demo
    WIFI_CREDENTIALS = 16,       // Network configuration
    UPDATE_DISPLAY = 25,         // Screen content control
    // ... and 20+ more types
}
```

## Network State Management

### Network State Manager (`network_state_manager.h/cpp`)
- **Connection tracking**: WiFi + WebSocket status monitoring
- **State transitions**: Handles connection/disconnection events  
- **Retry logic**: Exponential backoff for failed connections
- **Status reporting**: Network state updates to UI and logs

### Connection States
1. **Disconnected**: No network connectivity
2. **WiFi connecting**: Attempting WiFi association
3. **WiFi connected**: Local network access only
4. **WebSocket connecting**: Establishing server connection
5. **Fully connected**: Complete remote communication

## Data Exchange

### Send Sensor Data (`send_sensor_data.h/cpp`)
- **Sensor data streaming**: Real-time robot telemetry
- **Batch updates**: Efficient multi-sensor data packets
- **Registration**: Robot identification and capability reporting
- **Status broadcasting**: Battery, connection, demo states

### Route Types
```cpp
enum class RouteType {
    REGISTER,                    // Robot registration
    SENSOR_DATA,                 // Real-time sensor readings
    BYTECODE_STATUS,             // User program execution state
    WIFI_CONNECTION_RESULT,      // Network setup results
    BATTERY_MONITOR_DATA_FULL,   // Complete battery information
    // ... additional route types
};
```

## Security & Reliability

### Certificate Management
- **Root CA pinning**: Embedded certificate for HTTPS/WSS
- **SSL/TLS encryption**: Secure data transmission
- **Certificate validation**: Prevents man-in-the-middle attacks

### Error Handling
- **Connection timeouts**: Configurable timeout periods
- **Retry mechanisms**: Progressive backoff strategies
- **Fallback modes**: Graceful degradation when network unavailable
- **Resource cleanup**: Proper connection resource management

## Task System Integration

### Network Management Task
- **8KB stack**: Heavy WiFi operation requirements
- **Core 1 execution**: Communication core assignment
- **Priority 2**: Below critical but above background tasks
- **Connection lifecycle**: Setup, maintenance, teardown

### Network Communication Task  
- **8KB stack**: WebSocket message processing
- **Core 1 execution**: Application core for responsiveness
- **Priority 3**: Real-time communication requirements
- **Message polling**: Continuous WebSocket monitoring

## Environment Configuration

### Build Environments
```cpp
// Local development
DEFAULT_ENVIRONMENT = "local"
DEFAULT_PIP_ID = "9YhsJ"

// Staging deployment  
DEFAULT_ENVIRONMENT = "staging"
DEFAULT_PIP_ID = "bax2P"

// Production release
DEFAULT_ENVIRONMENT = "production" 
DEFAULT_PIP_ID = "PmKJZ"
```

### Endpoint Management
- **Dynamic URLs**: Environment-based server selection
- **Local override**: Development server on local network
- **Secure connections**: HTTPS/WSS for staging and production
- **Certificate matching**: Environment-specific validation

## Usage Examples

### WiFi Connection
```cpp
WiFiManager::getInstance().connectToNetwork("SSID", "password");
// Automatic connection attempts and status reporting
```

### Message Sending
```cpp
SerialQueueManager::getInstance().queueMessage("Status update");
// Thread-safe message queuing with automatic routing
```

### WebSocket Communication
```cpp
WebSocketManager::getInstance().sendMessage(jsonData);
// Automatic connection management and retry logic
```
