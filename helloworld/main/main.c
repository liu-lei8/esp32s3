#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "ap3216.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    uint16_t ir, als, ps;
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
    ap3216c_init(i2c0_master);

    lcd_show_string(30, 30, "ESP32-S3", RED, WHITE, 32, 1);
    lcd_show_string(30, 70, "AP3216C TEST", RED, WHITE, 24, 1);
    lcd_show_string(30, 100, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);
    lcd_show_string(30, 140, "AP3216C Ready", RED, WHITE, 16, 1);
    lcd_show_string(30, 160, "IR:", RED, WHITE, 16, 1);
    lcd_show_string(30, 180, "ALS:", RED, WHITE, 16, 1);
    lcd_show_string(30, 200, "PS:", RED, WHITE, 16, 1);

    while(1)
    {
        ap3216c_read_data(&ir, &als, &ps);      /*读取间隔要大于112.5ms*/
        lcd_show_num(30 + 40, 160, ir, 4, BLUE, WHITE, 16, 0);
        lcd_show_num(30 + 40, 180, als, 5, BLUE, WHITE, 16, 0);
        lcd_show_num(30 + 40, 200, ps, 4, BLUE, WHITE, 16, 0);
        LED_TOGGLE();
        vTaskDelay(200);
    }
}