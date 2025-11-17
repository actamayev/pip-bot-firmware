#pragma once

#include <Arduino.h>
#include <esp_flash.h>
#include <esp_system.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include "networking/serial_queue_manager.h"

void print_flash_info();
