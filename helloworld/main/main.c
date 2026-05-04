#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "tp_sensor.h"
#include "nvs_flash.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    esp_err_t ret;
    float temp;

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
    temperature_sensor_init();

    lcd_show_string(30, 50, "ESP32-S3", RED, WHITE, 32, 1);
    lcd_show_string(30, 90, "TEMPERATURE TEST", RED, WHITE, 24, 1);
    lcd_show_string(30, 120, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);
    lcd_show_string(30, 140, "TEMPERATURE: 00.00C", BLUE, WHITE, 16, 1);

    while(1)
    {
        temp = sensor_get_temperature();

        if (temp < 0)
        {
            temp = -temp;
            lcd_show_string(30 + 12 * 8, 140, "-", BLUE, WHITE, 16, 0);
        }
        else
        {
            lcd_show_string(30 + 12 * 8, 140, " ", BLUE, WHITE, 16, 0);
        }

        /*显示整数部分*/
        lcd_show_num(30 + 13 * 8, 140, temp, 2, BLUE, WHITE, 16, 0);
        /*显示小数部分*/
        lcd_show_num(30 + 16 * 8, 140, (uint32_t)(temp * 100) % 100, 2, BLUE, WHITE, 16, 0);

        LED_TOGGLE();
        vTaskDelay(200);
    }

}