#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "my_spiffs.h"
#include "nvs_flash.h"
#include "my_spiffs.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    spi2_init();
    xl9555_init(i2c0_master);
    lcd_init();
    ESP_ERROR_CHECK(spiffs_init("storage", DEFAULT_MOUNT_POINT, DEFAULT_FD_NUM));

    lcd_show_string(30, 50, "ESP32-S3", RED, WHITE, 32, 1);
    lcd_show_string(30, 90, "SPIFFS TEST", RED, WHITE, 24, 1);
    lcd_show_string(30, 120, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);
    lcd_show_string(30, 140, "Read Text:", RED, WHITE, 16, 1);

    spiffs_test();

    while(1)
    {
        LED_TOGGLE();
        vTaskDelay(500);
    }
}