#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"

uint32_t rng_get_random_num(void);              /*得到随机数*/
int rng_get_random_range(int min, int max);     /*得到某个范围的随机数*/