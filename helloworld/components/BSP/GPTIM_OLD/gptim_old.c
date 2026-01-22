#include "gptim_old.h"

static bool IRAM_ATTR timer_group_isr_callback(void *arg)
{
    timg_config_t* user_data = (timg_config_t*) arg;
    user_data->timer_count_value = 0;
    user_data->timer_count_value = timer_group_get_counter_value_in_isr(user_data->timer_group, user_data->timer_idx);

    LED_TOGGLE();

    if (!user_data->auto_reload)
    {
        user_data->alarm_value += user_data->timing_time;
        timer_group_set_alarm_value_in_isr(user_data->timer_group, user_data->timer_idx, user_data->alarm_value);
    }
    return 1;
}

void timg_init(timg_config_t* timg_config)
{
    uint32_t clk_src_hz = 0;
    esp_clk_tree_src_get_freq_hz(timg_config->clk_src, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &clk_src_hz);

    timer_config_t timer_config = {
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = timg_config->auto_reload,
        .clk_src = timg_config->clk_src,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .divider = clk_src_hz / 1000000
    };
    ESP_ERROR_CHECK(timer_init(timg_config->timer_group, timg_config->timer_idx, &timer_config));

    ESP_ERROR_CHECK(timer_set_counter_value(timg_config->timer_group, timg_config->timer_idx, 0));

    ESP_ERROR_CHECK(timer_set_alarm_value(timg_config->timer_group, timg_config->timer_idx, timg_config->alarm_value));

    ESP_ERROR_CHECK(timer_enable_intr(timg_config->timer_group, timg_config->timer_idx));

    ESP_ERROR_CHECK(timer_isr_callback_add(timg_config->timer_group, timg_config->timer_idx, timer_group_isr_callback, timg_config, 0));

    ESP_ERROR_CHECK(timer_start(timg_config->timer_group, timg_config->timer_idx));

}