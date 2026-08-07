#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "myi2s.h"
#include "audioplay.h"
#include "my_es8311.h"
#include "spi_sd.h"

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

    /* 先单独初始化 I2S，确保成功 */
    while (myi2s_init()) {
        lcd_show_string(30, 110, "I2S Init Error!", RED, WHITE, 16, 0);
        vTaskDelay(200);
        lcd_fill(30, 110, 240, 126, WHITE);
        vTaskDelay(200);
    }

    /* 再单独初始化 ES8311 */
    while (es8311_init(i2c0_master)) {
        lcd_show_string(30, 130, "ES8311 Init Error!", RED, WHITE, 16, 0);
        vTaskDelay(200);
        lcd_fill(30, 130, 240, 146, WHITE);
        vTaskDelay(200);
    }

    while (sd_spi_init())
    {
        lcd_show_string(30, 110, "SD Card Init Error!", RED, WHITE, 16, 0);
        vTaskDelay(200);
        lcd_show_string(30, 110, "Please Check!", RED, WHITE, 16, 0);
        vTaskDelay(200);
    }
    ret = exfuns_init();
    
    while (fonts_init())        /*检查字库*/
    {
        lcd_clear(WHITE);
        lcd_show_string(30, 30, "ESP32-S3", RED, WHITE, 16, 1);

        ret = fonts_update_font(30, 50, 16, (uint8_t*)"0:", RED, WHITE);

        while (ret)
        {
            lcd_show_string(30, 50, "Font Update Fail!", RED, WHITE, 16, 0);
            vTaskDelay(1000);
        }

        lcd_show_string(30, 50, "Font Update Success!", RED, WHITE, 16, 0);
        vTaskDelay(1000);
        lcd_clear(WHITE);
    }

    text_show_string(30, 50, 200, 16, "正点原子ESP32-S3开发板", 16, 1, RED, WHITE);
    text_show_string(30, 70, 200, 16, "音乐播放器实验", 16, 1, RED, WHITE);
    text_show_string(30, 90, 200, 16, "作者@柳磊", 16, 1, RED, WHITE);
    text_show_string(30, 110, 200, 16, "KEY0:NEXT  KEY1:PREV", 16, 1, RED, WHITE);
    text_show_string(30, 130, 200, 16, "KEY3:PAUSE/PLAY", 16, 1, RED, WHITE);

    vTaskDelay(500);

    while (1)
    {
        audio_play();
    }
}