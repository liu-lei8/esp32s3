#include "esptim.h"

void systimer_init(uint64_t tps)
{
    esp_timer_handle_t timer_handle;

    esp_timer_create_args_t timer_args = {
        .arg = NULL,
        .callback = systimer_callback,
    };
    esp_timer_create(&timer_args, &timer_handle);
    esp_timer_start_periodic(timer_handle, tps);
    
}

void systimer_callback(void* arg)
{
    LED_TOGGLE();
}