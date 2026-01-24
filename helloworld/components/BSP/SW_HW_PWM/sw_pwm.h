#pragma once

#include "driver/ledc.h"

#define LEDC_PWM_TIMER          LEDC_TIMER_1
#define LEDC_PWM_CH0_GPIO       GPIO_NUM_1
#define LEDC_PWM_CH0_CHANNEL    LEDC_CHANNEL_1

/*时钟源+定时器+通道号+GPIO引脚（4）+频率+分辨率+占空比值（3）*/
typedef struct {
    ledc_clk_cfg_t      ledc_clk;
    ledc_timer_t        ledc_timer;
    ledc_channel_t      ledc_channel;
    int                 gpio_num;
    uint32_t            freq_hz;
    ledc_timer_bit_t    duty_resolution;
    uint32_t            duty;
}ledc_config_t;

uint32_t ledc_duty_pow(uint32_t duty, uint8_t m, uint8_t n);
void ledc_init(ledc_config_t* ledc_cfg);
void ledc_pwm_set_duty(ledc_config_t* ledc_cfg, uint32_t duty);