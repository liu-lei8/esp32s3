#include "led.h"

void led_init(void)
{
    gpio_config_t led_init_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1ull << LED_GPIO_PIN,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };

    gpio_config(&led_init_cfg);
    LED(1); /*灌入电流接法，输出高电压，关闭LED*/
}