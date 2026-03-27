#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
//#include "oledfont.h"
#include "iic.h"
#include "oled.h"
#include "led.h"

void app_main(void)
{
    uint8_t t = 0;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    i2c_obj_t i2c0_master = iic_init(I2C_NUM_0);
    iic_scan(i2c0_master);
    oled_init(i2c0_master);

    oled_show_string(0, 0, "ALIENTEK", 24);
    oled_show_string(0, 24, "0.96' OLED TEST", 16);
    oled_show_string(0, 40, "LIULEI 2026/3/24", 12);
    oled_show_string(0, 52, "ASCII:", 12);
    oled_show_string(64, 52, "CODE:", 12);
    oled_reflash_gram();            /*更新显示到oled*/

    t = ' ';
    while(1)
    {
        oled_show_char(36, 52, t, 12, 1);
        oled_show_num(94, 52, t, 12);
        oled_reflash_gram();
        t++;
        if (t > '~')
        {
            t = ' ';
        }
        vTaskDelay(1000);
        LED_TOGGLE();
    }
}