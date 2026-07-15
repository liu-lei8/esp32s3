#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "key.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "spi_sd.h"
#include "fonts.h"
#include "text.h"
#include "nvs_flash.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    esp_err_t ret;
    uint8_t key = 0;
    uint32_t fontcnt = 0;
    uint8_t fontx[2] = {0};
    uint8_t t = 0;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    key_init();
    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    spi2_init();
    xl9555_init(i2c0_master);
    lcd_init();

    while (sd_spi_init())
    {
        lcd_show_string(30, 110, "SD Card Error!", RED , WHITE, 16, 0);
        vTaskDelay(500);
        lcd_show_string(30, 130, "Please Check!", RED, WHITE, 16, 0);
        vTaskDelay(500);
    }

    while (fonts_init())    /*检查字库*/
    {
UPD:
        lcd_clear(WHITE);
        
        lcd_show_string(30, 30, "ESP32-S3", RED, WHITE, 16, 1);

        key = fonts_update_font(30, 50, 16, (uint8_t*)"0:", RED, WHITE);  /*更新字库*/

        if (key)
        {
            lcd_show_string(30, 50, "Font Updata Failed!", RED, WHITE, 16, 1);
            return;
        }

        lcd_show_string(30 , 50, "Font Update Success!", RED, WHITE, 16, 1);
        vTaskDelay(1000);
        lcd_clear(WHITE);
    }

    unigbk_test();

    lcd_clear(WHITE);
    vTaskDelay(1000);
    text_show_string(30, 30, 200, 16, "正点原子ESP32-S3开发板", 16, 1, RED, WHITE);
    text_show_string(30, 50, 200, 16, "GBK字库测试程序", 16, 1, RED, WHITE);
    text_show_string(30, 70, 200, 16, "AUTHOR@LIU-LEI", 16, 1, RED, WHITE);
    text_show_string(30, 90, 200, 16, "BOOT: 更新字库", 16, 1, RED, WHITE);

    text_show_string(30, 110, 200, 16, "内码高字节:", 16, 1, BLUE, WHITE);
    text_show_string(30, 130, 200, 16, "内码低字节:", 16, 1, BLUE, WHITE);
    text_show_string(30, 150, 200, 16, "汉字计数器:", 16, 1, BLUE, WHITE);

    text_show_string(30, 180, 200, 24, "对应汉字为:", 24, 1, BLUE, WHITE);
    text_show_string(30, 210, 200, 16, "对应汉字为:", 16, 1, BLUE, WHITE);
    text_show_string(30, 230, 200, 16, "对应汉字为:", 12, 1, BLUE, WHITE);

    while (1)
    {
        fontcnt = 0;
        
        for (uint8_t i = 0x81; i < 0xFF; i++)         /* GBK内码高字节范围为0x81~0xFE */
        {
            fontx[0] = i;
            lcd_show_num(30 + 88, 110, i, 3, BLUE, WHITE, 16, 0);   /*显示内码高字节 */

            for (uint8_t j = 0x40; j < 0xFF; j++)
            {
                if (j == 0x7F) continue;

                fontcnt++;
                lcd_show_num(30 + 88, 130, j, 3, BLUE, WHITE, 16, 0);   /*显示内码低字节*/
                lcd_show_num(30 + 88, 150, fontcnt, 5, BLUE, WHITE, 16, 0); /*汉字计数显示*/
                fontx[1] = j;
                text_show_font(30 + 132, 180, fontx, 24, 0, BLUE, WHITE);
                text_show_font(30 + 88, 210, fontx, 16, 0, BLUE, WHITE);
                text_show_font(30 + 66, 230, fontx, 12, 0, BLUE, WHITE);

                t = 200;

                while ((t--) != 0)      /*延时，同时扫描按键*/
                {
                    vTaskDelay(pdMS_TO_TICKS(1));

                    key = key_scan(0);

                    if (key == BOOT_PRES)
                    {
                        free(unigbk_table);
                        goto UPD;       /*跳转到UPD位置（强制更新字库）*/
                    }
                }
                LED_TOGGLE();
            }
        }
        
    }
}