#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "dht11.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    uint8_t err;
    uint8_t t;
    float temp;
    float humi;

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

    lcd_show_string(30, 50, "ESP32-S3", RED, WHITE, 32, 1);
    lcd_show_string(30, 90, "DHT11 TEST", RED, WHITE, 24, 1);
    lcd_show_string(30, 120, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);

    err = dht11_init();
    if (err != 0)
    {
        lcd_show_string(30, 140, "DHT11 Init Error", RED, WHITE, 16, 1);
        return;
    }
    lcd_show_string(30, 140, "DHT11 OK!", RED, WHITE, 16, 1);
    lcd_show_string(30, 160, "Temp:      C", BLUE, WHITE, 16, 1);
    lcd_show_string(30, 180, "Humi:      %", BLUE, WHITE, 16, 1);

    while (1)
    {
        if (t % 10 == 0)
        {
            dht11_read_data(&temp, &humi);
            lcd_show_num(70, 160, (uint32_t)temp, 2, BLUE, WHITE, 16, 0);
            lcd_show_char(86, 160, '.', BLUE, WHITE, 16, 1);
            temp = (uint32_t)(temp * 1000) % 1000;
            lcd_show_num(94, 160, temp, 3, BLUE, WHITE, 16, 0);

            lcd_show_num(70, 180, (uint32_t)humi, 2, BLUE, WHITE, 16, 0);
            lcd_show_char(86, 180, '.', BLUE, WHITE, 16, 1);
            humi = (uint32_t)(humi * 1000) % 1000;
            lcd_show_num(94, 180, humi, 3, BLUE, WHITE, 16, 0);
        }

        vTaskDelay(10);
        t++;
        if (t == 20)
        {
            t = 0;
            LED_TOGGLE();
        }
    }
}