#ifndef SHOW_CHIP_INFO_H
#define SHOW_CHIP_INFO_H
#include <Arduino.h>
#include <esp_flash.h>
#include <esp_system.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

void printFlashInfo();
void testPSRAM();

#endif
