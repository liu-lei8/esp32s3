#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#define LED_GPIO    GPIO_NUM_1

#define FULL_EVENT_BIT BIT0
#define EMPTY_EVENT_BIT BIT1

EventGroupHandle_t ledc_event_handle = NULL;

bool IRAM_ATTR ledc_finish_cb(const ledc_cb_param_t *param, void *user_arg)
{   
    BaseType_t task_woken = pdFALSE;
    if (param->duty)
    {
        xEventGroupSetBitsFromISR(ledc_event_handle, FULL_EVENT_BIT, &task_woken);
    }
    else
    {
        xEventGroupSetBitsFromISR(ledc_event_handle, EMPTY_EVENT_BIT, &task_woken);
    }

    return task_woken;
}

void led_run(void)
{
    EventBits_t ev;
   
    ledc_cbs_t ledc_cb = {
        .fade_cb = ledc_finish_cb,
    };
    ledc_cb_register(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, &ledc_cb, NULL);

    while (1) 
    {
        ev = xEventGroupWaitBits(ledc_event_handle, FULL_EVENT_BIT | EMPTY_EVENT_BIT, pdTRUE, pdFALSE, 5000);
        if (ev & FULL_EVENT_BIT)
        {
            ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 2000);
            ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
        }
        if (ev & EMPTY_EVENT_BIT)
        {
            ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4095, 2000);
            ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
        }
    }
    
}

void app_main(void)
{
    gpio_config_t led_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1 << LED_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&led_cfg);

    ledc_timer_config_t ledc_timer = {
        .clk_cfg = LEDC_AUTO_CLK,
        .duty_resolution = LEDC_TIMER_12_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel ={
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = LED_GPIO,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
    }; 
    ledc_channel_config(&ledc_channel);

    ledc_fade_func_install(0);
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4095, 2000);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);

    ledc_cbs_t ledc_cb ={
        .fade_cb = ledc_finish_cb,
    };
    ledc_cb_register(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, &ledc_cb, NULL);

    ledc_event_handle = xEventGroupCreate();
    
    xTaskCreatePinnedToCore(led_run, "led_run", 2048, NULL, 3, NULL, 1);
}