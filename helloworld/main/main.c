#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gptim_new.h"
#include "esp_log.h"
#include "nvs_flash.h"

void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    user_gptimer_config_t* user_gptimer_cfg = malloc(sizeof(user_gptimer_config_t));

    user_gptimer_cfg->clk_src = GPTIMER_CLK_SRC_APB;
    user_gptimer_cfg->timing_time = 1 * 1000000;
    user_gptimer_cfg->alarm_value = user_gptimer_cfg->timing_time;
    user_gptimer_cfg->gptimer_count_value = 0;
    user_gptimer_cfg->gptimer_handle = NULL;
    user_gptimer_cfg->auto_reload = false;

    led_init();
    user_gptimer_init(user_gptimer_cfg);
    
    ESP_LOGI("GPTimer", "GPTimer initialized with auto reload");
    while (1)
    {
        if (user_gptimer_cfg->gptimer_count_value != 0)
        {
            ESP_LOGI("GPTimer", "gptimer count value: %llu", user_gptimer_cfg->gptimer_count_value);
            user_gptimer_cfg->gptimer_count_value = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    free(user_gptimer_cfg);
}