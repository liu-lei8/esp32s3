#include "led.h"
#include "spi.h"
#include "lcd.h"
#include "xl9555.h"
#include "iic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

void app_main(void)
{
    esp_err_t ret;
    i2c_obj_t iic0_master;
    uint8_t x = 0;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    iic0_master = iic_init(I2C_NUM_0);
    iic_scan(iic0_master);
    xl9555_init(iic0_master);
    gpio_intr_disable(XL9555_INT_IO);
    spi2_init();
    lcd_init();

    while(1)
    {
        switch(x)
        {
            case 0: lcd_clear(WHITE); break;
            case 1: lcd_clear(BLACK); break;
            case 2: lcd_clear(BLUE); break;
            case 3: lcd_clear(RED); break;
            case 4: lcd_clear(MAGENTA); break;
            case 5: lcd_clear(GREEN); break;
            case 6: lcd_clear(CYAN); break;
            case 7: lcd_clear(YELLOW); break;
            case 8: lcd_clear(BRRED); break;
            case 9: lcd_clear(GRAY); break;
            case 10: lcd_clear(LGRAY); break;
            case 11: lcd_clear(BROWN); break;
            default: break;
        }
        // lcd_show_string(10, 40, "ESP32", RED, WHITE, 32, 0);
        // lcd_show_string(10, 80, "SPI LCD TEST", RED, WHITE, 24, 0);
        // lcd_show_string(10, 110, "AUTHOR@LIU-LEI", RED, WHITE, 16, 0);
        lcd_show_chinese_string(20, 50, "薛春威", RED, WHITE, 32, 1);
        lcd_show_chinese_string(52, 90, "小南娘", RED, WHITE, 32, 1);
        lcd_show_chinese_string(20, 130, "哈哈哈哈哈哈！", RED, WHITE, 32, 1);
        vTaskDelay(1000);
        lcd_show_picture(0, WHITE, RED, 0);
        x++;
        if (x == 12)
        {
            x = 0;
        }
        LED_TOGGLE();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}