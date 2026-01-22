#include "key.h"

void key_init(void)
{
    gpio_config_t key_init_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ull << KEY_GPIO_PIN,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&key_init_cfg);

}

uint8_t key_scan(uint8_t mode)
{
    uint8_t keyval = 0;
    static uint8_t key_press = 1;  /*按键松开标志*/

    if (mode)
    {
        key_press = 1;
    }

    if (key_press && !BOOT)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        key_press = 0;

        if (!BOOT)
        {
            keyval = BOOT_PRES;
        }
    }
    else if(BOOT)
    {
        key_press = 1;
    }

    return keyval;
}