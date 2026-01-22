#include "exit.h"

esp_timer_handle_t debounce_timer;

static uint32_t last_intrrupt_time = 0;

static void IRAM_ATTR exit_callback(void* args)
{
    uint32_t now = esp_timer_get_time() / 1000;         /*转化为ms*/
    if (now - last_intrrupt_time > DEBOUNCE_TIME_MS)
    {
        last_intrrupt_time = now;
        esp_timer_start_once(debounce_timer, DEBOUNCE_TIME_MS * 1000);  /*转化为us*/
    }
}

static void timer_callback(void* args)
{
    if (!BOOT)
    {
        LED_TOGGLE();
    }
}

void exit_init(void)
{
    gpio_config_t exit_cfg = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ull << EXIT_GPIO_PIN,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&exit_cfg);

    esp_timer_create_args_t timer_args = {
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .callback = timer_callback,
        .name = "debounce_timer"
    };
    esp_timer_create(&timer_args, &debounce_timer);

    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    gpio_isr_handler_add(EXIT_GPIO_PIN, exit_callback, NULL);
    gpio_intr_enable(EXIT_GPIO_PIN);
}