#pragma once

#include <esp32-hal-timer.h>
#include "actuators/led/rgb_led.h"
#include "actuators/led/led_animations.h"
#include "networking/serial_queue_manager.h"
#include "actuators/buttons.h"

bool holdToWake();
