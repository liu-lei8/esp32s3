#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "wifi_scan.h"

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

    lcd_show_string(10, 50, "ESP32-S3", RED, WHITE, 16, 1);
    lcd_show_string(10, 70, "WIFI SCAN TEST", RED, WHITE, 16, 1);
    lcd_show_string(10, 90, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);
    lcd_draw_line(120, 0, 120, 280, DARKBLUE);
    
    wifi_scan();

    while (1)
    {
        LED_TOGGLE();
        vTaskDelay(500);
    }
}