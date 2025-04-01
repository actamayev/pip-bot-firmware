#pragma once
#include "./utils.h"
#include "../sensors/sensors.h"
#include "../actuators/buttons.h"
#include "../sensors/encoder_manager.h"
#include "../actuators/display_screen.h"

void multizoneTofLogger();
void imuLogger();
void sideTofsLogger();
void setupButtonLoggers();
void log_motor_rpm();
