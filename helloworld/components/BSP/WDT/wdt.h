#pragma once

#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_system.h"

#define BOOT_INT_GPIO_PIN   GPIO_NUM_0
#define BOOT                gpio_get_level(BOOT_INT_GPIO_PIN)

void wdt_init(uint64_t tps);
void restart_timer(uint64_t timeout);