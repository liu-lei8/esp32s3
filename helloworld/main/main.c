#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "iic.h"
#include "ltdc.h"
#include "touch.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    ltdc_init();
    tp_dev.init(i2c0_master);

    load_draw_dialog();
    ctp_test();
}