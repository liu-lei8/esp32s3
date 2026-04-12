#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "esp_rtc.h"

i2c_obj_t i2c0_master;
const char* weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saterday"};

void app_main(void)
{
    esp_err_t ret;
    uint8_t tbuf[40];
    uint8_t t = 0;

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
    rtc_set_time(2026, 4, 11, 19, 53, 0);

    lcd_show_string(10, 40, "ESP32", RED, WHITE, 32, 1);
    lcd_show_string(10, 80, "RTC Test", RED, WHITE, 24, 1);
    lcd_show_string(10, 110, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);

    while(1)
    {
        t++;
        if(t % 100 == 0)     /*每1000ms打印一次*/
        {
            rtc_get_time();

            sprintf((char*)tbuf, "Time: %02d:%02d:%02d", calendar.hour, calendar.min, calendar.sec);
            printf("Time: %02d:%02d:%02d\n", calendar.hour, calendar.min, calendar.sec);
            lcd_show_string(10, 130, (char*)tbuf, BLUE, WHITE, 16, 0);

            sprintf((char*)tbuf, "Date: %04d-%02d-%02d", calendar.year, calendar.month, calendar.date);
            printf("Date: %04d-%02d-%02d\n", calendar.year, calendar.month, calendar.date);
            lcd_show_string(10, 150, (char*)tbuf, BLUE, WHITE, 16, 0);

            sprintf((char*)tbuf, "Week: %s", weekdays[calendar.week]);
            printf("Week: %s\n", weekdays[calendar.week]);
            lcd_show_string(10, 170, (char*)tbuf, BLUE, WHITE, 16, 0);
            t = 0;
        }

        if (t % 50 == 0)    /*每500ms翻转一次led*/
        {
            LED_TOGGLE();
        }
        vTaskDelay(10);
    }
}