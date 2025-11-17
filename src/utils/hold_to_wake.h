#pragma once

#include <esp32-hal-timer.h>

#include "actuators/buttons.h"
#include "actuators/led/led_animations.h"
#include "actuators/led/rgb_led.h"
#include "networking/serial_queue_manager.h"

bool hold_to_wake();
