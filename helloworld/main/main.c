#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "led.h"
#include "ds18b20.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    esp_err_t ret;
    uint8_t err;
    uint8_t t = 0;
    short temp0 = 0;
    short temp1 = 0;

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

    lcd_show_string(30, 50, "ESP32-S3", RED, WHITE, 32, 1);
    lcd_show_string(30, 90, "DS18B20 TEST", RED, WHITE, 24, 1);
    lcd_show_string(30, 120, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);

    err = ds18b20_init();
    if (err)
    {
        lcd_show_string(30, 140, "DS18B20 Check Error", RED, WHITE, 16, 1);
        return;
    }
    //ds18b20_read_rom(); /*单总线上只能有一个传感器*/

    lcd_show_string(30, 140, "DS18B20 OK", RED, WHITE, 16, 1);
    lcd_show_string(30, 160, "temp0: 00.0C", BLUE, WHITE, 16, 0);
    lcd_show_string(30, 180, "temp1: 00.0C", BLUE, WHITE, 16, 0);
    while (1)
    {
        if (t % 10 == 0)
        {
            ds18b20_start_convert();
            temp0 = ds18b20_get_temperature_rom(ds18b20_rom0);   /*该函数中有750ms的延迟*/
            temp1 = ds18b20_get_temperature_rom(ds18b20_rom1);
            if (temp0 < 0)
            {
                lcd_show_string(30 + 6 * 8, 160, "-", BLUE, WHITE, 16, 0);
                lcd_show_string(30 + 6 * 8, 180, "-", BLUE, WHITE, 16, 0);
                temp0 = -temp0;   /*转为正数*/
                temp1 = -temp1;
            }
            else
            {
                lcd_show_string(30 + 6 * 8, 160, " ", BLUE, WHITE, 16, 0);
                lcd_show_string(30 + 6 * 8, 180, " ", BLUE, WHITE, 16, 0);
            }

            lcd_show_num(30 + 7 * 8, 160, temp0 / 10, 2, BLUE, WHITE, 16, 0);/*显示整数*/
            lcd_show_num(30 + 10 * 8, 160, temp0 % 10, 1, BLUE, WHITE, 16, 0);/*显示小数*/
            lcd_show_num(30 + 7 * 8, 180, temp1 / 10, 2, BLUE, WHITE, 16, 0);/*显示整数*/
            lcd_show_num(30 + 10 * 8, 180, temp1 % 10, 1, BLUE, WHITE, 16, 0);/*显示小数*/
        }

        vTaskDelay(10);
        ++t;

        if (t == 20)
        {
            LED_TOGGLE();
            t = 0;
        }
    }
}