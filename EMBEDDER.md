**EMBEDDER PROJECT CONTEXT**
<OVERVIEW>
Name            = MyProject
Target MCU      = ESP_32_S3
Toolchain       = Arduino
Debug Interface = serial
RTOS / SDK      = freertos
Project Summary = Firmware for an educational robot
</OVERVIEW>

<COMMANDS>
# --- Build / Compile --------------------------------------------------------
build_command         = platformio run --environment local

# --- Flash ------------------------------------------------------------------
flash_command         =  platformio run --target upload --target monitor --environment local --upload-port /dev/cu.usbmodem101 --monitor-port /dev/cu.usbmodem101esp32-s3-devkitc-1

# --- Debug ------------------------------------------------------------------
gdb_server_command    = <optional, e.g., openocd ... or ST-LINK_gdbserver ...>
gdb_server_host       = localhost
gdb_server_port       = 61234
gdb_client_command    = arm-none-eabi-gdb
target_connection     = remote

# --- Serial Monitor ----------------------------------------------------------
# Option 1: For custom commands (waf, make, etc.)
# serial_monitor_command = source .venv/bin/activate && ./waf console --tty /dev/ttyACM0
# serial_monitor_interactive = true
# serial_startup_commands = [reset]

# Option 2: Direct port/baudrate (use {port} and {baud} placeholders)
serial_monitor_interactive = false
serial_encoding       = ascii
serial_port = /dev/cu.usbmodem101
serial_baudrate = 115200
