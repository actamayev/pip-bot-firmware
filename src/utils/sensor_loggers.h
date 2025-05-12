#pragma once
#include "utils.h"
#include "actuators/buttons.h"
#include "sensors/encoder_manager.h"
#include "actuators/display_screen.h"
#include "sensors/multizone_tof_sensor.h"
#include "sensors/imu.h"
#include "sensors/side_tof_manager.h"

void multizoneTofLogger();
void imuLogger();
void sideTofsLogger();
void setupButtonLoggers();
void log_motor_rpm();
