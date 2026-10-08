#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "wifi_sta.h"
#include "lwip_tcpclient.h"
#include "camera.h"

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

    lcd_show_string(30, 50, "ESP32-S3", RED, WHITE, 16, 1);
    lcd_show_string(30, 70, "WIFI CAMERA TEST", RED, WHITE, 16, 1);
    lcd_show_string(30, 90, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);

    /* wifi sta模式初始化 */
    wifi_sta_init();
    esp_wifi_set_ps(WIFI_PS_NONE);  /* Power Save关闭省电模式，防止上传像素数据周期性卡顿 */

    /* OV5640摄像头初始化 */
    ret = camera_init();
    while (ret)
    {
        lcd_show_string(30, 110, "camera init fail!", RED, WHITE, 16, 0);
        vTaskDelay(1000);
    }

    /* lwip测试代码 */
    lwip_tcpclient_demo();
}
