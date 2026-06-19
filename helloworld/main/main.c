#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "spi_sd.h"
#include "nvs_flash.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    esp_err_t ret;
    size_t total_KB;
    size_t free_KB;

    printf("SPI2_HOST = %d, SPI3_HOST = %d\n", SPI2_HOST, SPI3_HOST);

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
    ret = sd_spi_init();        /*自动初始化spi总线*/


    lcd_show_string(30, 50, "ESP32-S3", RED, WHITE, 32, 1);
    lcd_show_string(30, 90, "SD FATFS TEST", RED ,WHITE, 24, 1);
    lcd_show_string(30, 120, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);

    while (ret)       /*检测不到SD卡*/
    {
        lcd_show_string(30, 140, "SD Card Error!", RED, WHITE, 16, 0);
        lcd_show_string(30, 160, "Please Check!", RED ,WHITE, 16, 0);
        LED_TOGGLE();
        vTaskDelay(500);
    }


    lcd_show_string(30, 140, "SD Card OK!", RED ,WHITE, 16, 1);
    lcd_show_string(30, 160, "Total:     MB", BLUE, WHITE, 16, 1);
    lcd_show_string(30, 180, "Free :     MB", BLUE, WHITE, 16, 1);

    sd_get_fatfs_usage(&total_KB, &free_KB);
    printf("total_MB:%d, free_MB:%d\n", total_KB / 1024, free_KB / 1024);
    lcd_show_num(30 + 48, 160, total_KB / 1024, 4, BLUE, WHITE, 16, 0);
    lcd_show_num(30 + 48, 180, free_KB / 1024, 4, BLUE, WHITE, 16, 0);

    while (1)
    {
        LED_TOGGLE();
        vTaskDelay(500);
    }

}