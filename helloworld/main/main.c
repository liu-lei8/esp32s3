#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "camera.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    uint8_t t = 0;
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    spi2_init();
    xl9555_init(i2c0_master);
    lcd_init();
    ret = camera_init();

    lcd_show_string(30, 50, "ESP32-S3", RED, WHITE, 32, 1);
    lcd_show_string(30, 90, "Camera Test", RED, WHITE, 24, 1);
    lcd_show_string(30, 120, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);

    while (ret != ESP_OK)
    {
        lcd_show_string(30, 140, "Camera Fail!", RED, WHITE, 16, 0);
        vTaskDelay(500);
    }
    
    lcd_clear(BLACK);

    while(1)
    {
        camera_show(0, 0);

        t++;
        if (t % 20 == 0)
        {
            LED_TOGGLE();
            t = 0;
        }
        vTaskDelay(5);
    }
}