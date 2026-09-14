#pragma once /* esptim_video.h */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

extern esp_timer_handle_t esp_tim_handle;

/**
 * @brief 初始化系统定时器
 * @param tp：定时器周期时间us
 */
void esptime_int_init(uint64_t tp);

/* 定时器函数 */
void TIM_PeriodElapsedCallback(void* arg);