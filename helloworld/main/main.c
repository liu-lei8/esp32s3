#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "key.h"
#include "rng.h"
#include "nvs_flash.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    uint8_t key;
    uint32_t random;
    uint8_t t = 0;

    esp_err_t ret;
    ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    spi2_init();
    xl9555_init(i2c0_master);
    lcd_init();
    key_init();

    lcd_show_string(30, 40, "ESP32-S3", RED, WHITE, 32, 1);
    lcd_show_string(30, 80, "RNG TEST", RED, WHITE, 24, 1);
    lcd_show_string(30, 110, "AUTHOR@LIU LEI", RED, WHITE, 16, 1);

    lcd_show_string(30, 130, "RNG Ready!", RED, WHITE, 16, 1);
    lcd_show_string(30, 150, "Boot:Get Random Number", RED, WHITE, 16, 1);
    lcd_show_string(30, 170, "Random Num:", RED, WHITE, 16, 1);
    lcd_show_string(30, 190, "Random Num[0-9]:", RED, WHITE, 16, 1);

    while (1)
    {
        key = key_scan(0);
        if (key == BOOT_PRES)
        {
            random = rng_get_random_num();
            lcd_show_num(30 + 11 * 8, 170, random, 8, BLUE, WHITE, 16, 0);
        }

        if (t % 20 == 0)
        {
            LED_TOGGLE();
            random = rng_get_random_range(0, 9);
            lcd_show_num(30 + 16 * 8, 190, random, 1, BLUE, WHITE, 16, 0);
        }
        t++;
        vTaskDelay(10);
    }
}