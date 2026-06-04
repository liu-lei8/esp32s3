#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ltdc.h"
#include "led.h"
#include "nvs_flash.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    esp_err_t ret;
    char lcd_id[14];
    uint8_t x = 0;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    ltdc_init();

    sprintf(lcd_id, "LCD ID:%#04x", ltdcdev.id);

    while (1)
    {
        switch (x)
        {
            case 0:ltdc_clear(WHITE); break;
            case 1:ltdc_clear(BLACK); break;
            case 2:ltdc_clear(RED); break;
            case 3:ltdc_clear(GREEN); break;
            case 4:ltdc_clear(BLUE); break;
            case 5:ltdc_clear(MAGENTA); break;
            case 6:ltdc_clear(YELLOW); break;
            case 7:ltdc_clear(CYAN); break;
        }

        ltdc_show_string(30, 50, "ESP32-S3", 32, RED, WHITE, 1);
        ltdc_show_string(30, 90, "LTDC TEST", 24, RED ,WHITE, 1);
        ltdc_show_string(30, 120, "AUTHOR@LIE-LEI", 16, RED, WHITE, 1);
        ltdc_show_string(30, 140, lcd_id, 16, RED, WHITE, 1);

        x++;
        if (x == 8)
        {
            x = 0;
        }

        LED_TOGGLE();
        vTaskDelay(1000);
    }
}