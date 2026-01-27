#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wdt.h"
#include "nvs_flash.h"
#include "led.h"
#include "key.h"

void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    key_init();
    wdt_init(1000000);
    LED(0);

    while (1)
    {
        if (key_scan(0) == BOOT_PRES)
        {
            restart_timer(1000000);
        }
        vTaskDelay(10);
    }
}