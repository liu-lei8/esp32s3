#include "gptim_new.h"

static bool gptimer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    user_gptimer_config_t* user_gptimer_cfg = (user_gptimer_config_t*) user_ctx;
    ESP_ERROR_CHECK(gptimer_get_raw_count(user_gptimer_cfg->gptimer_handle, &user_gptimer_cfg->gptimer_count_value));

    if (!user_gptimer_cfg->auto_reload)
    {
        user_gptimer_cfg->alarm_value += user_gptimer_cfg->timing_time;
        gptimer_alarm_config_t alarm_cfg = {
            .alarm_count = user_gptimer_cfg->alarm_value,
            .flags.auto_reload_on_alarm = user_gptimer_cfg->auto_reload,
            .reload_count = 0
        };
        gptimer_set_alarm_action(user_gptimer_cfg->gptimer_handle, &alarm_cfg);
    }

    LED_TOGGLE();

    return true;
}


void user_gptimer_init(user_gptimer_config_t* user_gptimer_cfg)
{
    uint32_t clk_src_hz;
    ESP_ERROR_CHECK(esp_clk_tree_src_get_freq_hz(user_gptimer_cfg->clk_src, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &clk_src_hz));

    gptimer_config_t gptimer_cfg = {
        .clk_src = user_gptimer_cfg->clk_src,
        .direction = GPTIMER_COUNT_UP,
        .flags.intr_shared = false,
        .resolution_hz = 1000000, 
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&gptimer_cfg, &user_gptimer_cfg->gptimer_handle));

    ESP_ERROR_CHECK(gptimer_set_raw_count(user_gptimer_cfg->gptimer_handle, user_gptimer_cfg->gptimer_count_value));

    gptimer_alarm_config_t alarm_cfg = {
        .alarm_count = user_gptimer_cfg->alarm_value,
        .flags.auto_reload_on_alarm = user_gptimer_cfg->auto_reload,
        .reload_count = 0,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(user_gptimer_cfg->gptimer_handle, &alarm_cfg));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = gptimer_callback,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(user_gptimer_cfg->gptimer_handle, &cbs, user_gptimer_cfg));

    ESP_ERROR_CHECK(gptimer_enable(user_gptimer_cfg->gptimer_handle));
    ESP_ERROR_CHECK(gptimer_start(user_gptimer_cfg->gptimer_handle));
}