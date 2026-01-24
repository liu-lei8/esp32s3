#include "sw_pwm.h"

uint32_t ledc_duty_pow(uint32_t duty, uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--)
    {
        result *= m;
    }
    return result * duty / 100;
}

void ledc_init(ledc_config_t* ledc_cfg)
{
    ledc_timer_config_t ledc_timer_cfg = {
        .clk_cfg = ledc_cfg->ledc_clk,
        .deconfigure = false,
        .duty_resolution = ledc_cfg->duty_resolution,
        .freq_hz = ledc_cfg->freq_hz,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = ledc_cfg->ledc_timer
    };
    ledc_timer_config(&ledc_timer_cfg);

    ledc_channel_config_t ledc_channel_cfg = {
        .channel = ledc_cfg->ledc_channel,
        .duty = ledc_cfg->duty,
        .flags.output_invert = false,
        .gpio_num = ledc_cfg->gpio_num,
        .hpoint = 0,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = ledc_cfg->ledc_timer
    };
    ledc_channel_config(&ledc_channel_cfg);
}

void ledc_pwm_set_duty(ledc_config_t* ledc_cfg, uint32_t duty)
{
    uint32_t resolution_duty = ledc_duty_pow(duty, 2, ledc_cfg->duty_resolution);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_cfg->ledc_channel, resolution_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_cfg->ledc_channel);
}