#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "qma6100p.h"
#include "led.h"
#include "nvs_flash.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    esp_err_t ret;
    uint8_t t = 0;
    qma6100p_rawdata_t data;
    char buf[10];

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    spi2_init();

    iic_scan(i2c0_master);

    xl9555_init(i2c0_master);
    lcd_init();
    qma6100p_init(i2c0_master);

    lcd_show_string(30, 30, "ESP32-S3", RED, WHITE, 32, 1);
    lcd_show_string(30, 70, "QMA6100P TEST", RED, WHITE, 24, 1);
    lcd_show_string(30, 100, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);
    lcd_show_string(30, 120, "ACC_X: ", RED, WHITE, 16, 1);
    lcd_show_string(30, 140, "ACC_Y: ", RED, WHITE, 16, 1);
    lcd_show_string(30, 160, "ACC_Z: ", RED, WHITE, 16, 1);
    lcd_show_string(30, 180, "Picth: ", RED, WHITE, 16, 1);
    lcd_show_string(30, 200, "Roll : ", RED, WHITE, 16, 1);

    while(1)
    {
        vTaskDelay(10);
        ++t;

        if (t == 20)
        {
            qma6100p_read_rawdata(&data);

            sprintf(buf, "%3.1f", data.acc_x);
            lcd_show_string(30 + 7 * 8, 120, buf, BLUE, WHITE, 16, 0);
            sprintf(buf, "%3.1f", data.acc_y);
            lcd_show_string(30 + 7 * 8, 140, buf, BLUE, WHITE, 16, 0);
            sprintf(buf, "%3.1f", data.acc_z);
            lcd_show_string(30 + 7 * 8, 160, buf, BLUE, WHITE, 16, 0);
            sprintf(buf, "%3.1f", data.pitch);
            lcd_show_string(30 + 7 * 8, 180, buf, BLUE, WHITE, 16, 0);
            sprintf(buf, "%3.1f", data.roll);
            lcd_show_string(30 + 7 * 8, 200, buf, BLUE, WHITE, 16, 0);

            t = 0;
            LED_TOGGLE();
        }
    }
}