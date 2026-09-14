#include "esptim_video.h"

uint8_t frameup;
esp_timer_handle_t esp_tim_handle;      /* 定时器函数回调句柄 */

void esptime_int_init(uint64_t tp)
{
    esp_timer_create_args_t tim_periodic_arg = {
        .callback = TIM_PeriodElapsedCallback,
        .arg = NULL,
    };

    esp_timer_create(&tim_periodic_arg, &esp_tim_handle);       /* 创建一个定时器事件 */
    esp_timer_start_periodic(esp_tim_handle, tp);               /* 每周期触发一次 */
}

void TIM_PeriodElapsedCallback(void* arg)
{
    frameup = 1;
}

