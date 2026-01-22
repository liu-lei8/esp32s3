#pragma once


#include "esp_timer.h"
#include "led.h"

void systimer_init(uint64_t tps);
void systimer_callback(void* arg);