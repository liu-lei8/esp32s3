#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hw_pwm.h"
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

    ledc_config_t1 ledc_cfg = {
        .duty = 0,
        .duty_resolution = 13,
        .freq_hz = 1000,
        .ledc_channel_num = LEDC_PWM_CH1_CHANNEL,
        .ledc_clk = LEDC_AUTO_CLK,
        .ledc_gpio = LEDC_PWM_CH1_GPIO,
        .ledc_timer_num = LEDC_PWM_TIMER1,
    };
    ledc_init1(&ledc_cfg);


    while (1)
    {
        vTaskDelay(10);
        ledc_pwm_set_fade(&ledc_cfg, 100);
    }
}