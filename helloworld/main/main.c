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
    int alarm_num = 0;
    char buf[40];
    const char* temp[] = {"temp0", "temp1"};
    int j = 0;

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
    //ds18b20_read_rom(); /*读取序列号，单总线上只能有一个传感器*/

    /*硬件设置报警*/
    ds18b20_set_alarm(sensor_A.rom, sensor_A.low_threshold / 10, sensor_A.high_threshold / 10);
    ds18b20_set_alarm(sensor_B.rom, sensor_B.low_threshold / 10, sensor_B.high_threshold / 10);

    lcd_show_string(30, 140, "DS18B20 OK", RED, WHITE, 16, 1);
    lcd_show_string(30, 160, "temp0: 00.0C (10-20C)", BLUE, WHITE, 16, 0);
    lcd_show_string(30, 180, "temp1: 00.0C (0-30C)", BLUE, WHITE, 16, 0);
    while (1)
    {
        if (t % 10 == 0)    /*显示温度数据*/
        {
            ds18b20_start_convert();    /*该函数中有750ms的延迟*/
            temp0 = ds18b20_get_temperature_rom(ds18b20_rom0);
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

        alarm_num = ds18b20_alarm_search();     /*寻找报警设备，并设置报警标志*/
        sprintf(buf, "%d ds18b20 alarm", alarm_num);
        lcd_show_string(30, 200, buf, RED, WHITE, 16, 0);
        for (int i = 0; i < DS18B20_COUNT; i++) /*扫描报警设备并依次打印到lcd*/
        {
            if (sensors[i]->alarm_flag)
            {
                ++j;    /*记录报警设备数量，便于逐行在lcd上显示*/
                if (sensors[i]->low_alarm)
                {
                    sprintf(buf, "%s low temp alarm! ", temp[i]);
                    lcd_show_string(30, 200 + 20 * j, buf, RED, WHITE, 16, 0);
                }
                else
                {
                    sprintf(buf, "%s high temp alarm!", temp[i]);
                    lcd_show_string(30, 200 + 20 * j, buf, RED, WHITE, 16, 0);
                }
                if (j >= alarm_num)
                {
                    j = 0;
                }
            }
        }

        for (uint8_t i = 0; i < DS18B20_COUNT; i++) /*清空报警标志*/
        {
            sensors[i]->alarm_flag = 0;
            sensors[i]->low_alarm = 0;
            sensors[i]->high_ararm = 0;
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