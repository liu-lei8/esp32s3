#pragma once

#include "driver/ledc.h"

#define LEDC_PWM_TIMER1         LEDC_TIMER_0
#define LEDC_PWM_CH1_GPIO       GPIO_NUM_1
#define LEDC_PWM_CH1_CHANNEL    LEDC_CHANNEL_0
#define LEDC_PWM_FADE_TIME      2000
#define LEDC_PWM_SPEED_MODE     LEDC_LOW_SPEED_MODE

typedef struct {
     ledc_clk_cfg_t ledc_clk;
     ledc_timer_t ledc_timer_num;
     ledc_channel_t ledc_channel_num;
     int ledc_gpio;
     ledc_timer_bit_t duty_resolution;
     uint32_t freq_hz;
     uint32_t duty;
}ledc_config_t1;

uint32_t ledc_duty_pow1(uint32_t duty, uint8_t m, uint8_t n);
void ledc_init1(ledc_config_t1* ledc_cfg);
void ledc_pwm_set_fade(ledc_config_t1* ledc_cfg, uint32_t duty);
