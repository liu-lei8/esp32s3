#include "hw_pwm.h"

uint32_t ledc_duty_pow1(uint32_t duty, uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--)
    {
        result *= m;
    }
    return result * duty / 100;
}
void ledc_init1(ledc_config_t1* ledc_cfg)
{
    ledc_timer_config_t ledc_timer_cfg = {
        .clk_cfg = ledc_cfg->ledc_clk,
        .deconfigure = false,
        .duty_resolution = ledc_cfg->duty_resolution,
        .freq_hz = ledc_cfg->freq_hz,
        .speed_mode = LEDC_PWM_SPEED_MODE,
        .timer_num = ledc_cfg->ledc_timer_num
    };
    ledc_timer_config(&ledc_timer_cfg);

    ledc_channel_config_t ledc_channel_cfg = {
        .channel = ledc_cfg->ledc_channel_num,
        .duty = ledc_cfg->duty,
        .flags.output_invert = false,
        .gpio_num = ledc_cfg->ledc_gpio,
        .hpoint = 0,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = LEDC_PWM_SPEED_MODE,
        .timer_sel = ledc_cfg->ledc_timer_num
    };
    ledc_channel_config(&ledc_channel_cfg);

    ledc_fade_func_install(0);
}

void ledc_pwm_set_fade(ledc_config_t1* ledc_cfg, uint32_t duty)
{
    uint32_t resolution_duty = ledc_duty_pow1(duty, 2, ledc_cfg->duty_resolution);

    ledc_set_fade_with_time(LEDC_PWM_SPEED_MODE, ledc_cfg->ledc_channel_num, resolution_duty, LEDC_PWM_FADE_TIME);
    ledc_fade_start(LEDC_PWM_SPEED_MODE, ledc_cfg->ledc_channel_num, LEDC_FADE_NO_WAIT);

    ledc_set_fade_with_time(LEDC_PWM_SPEED_MODE, ledc_cfg->ledc_channel_num, 0, LEDC_PWM_FADE_TIME);
    ledc_fade_start(LEDC_PWM_SPEED_MODE, ledc_cfg->ledc_channel_num, LEDC_FADE_NO_WAIT);

}