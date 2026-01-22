#pragma once

#include "esp_timer.h"
#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define EXIT_GPIO_PIN   GPIO_NUM_0
#define BOOT            gpio_get_level(EXIT_GPIO_PIN)

#define DEBOUNCE_TIME_MS    20

void exit_init(void);