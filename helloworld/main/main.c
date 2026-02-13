#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soft_iic.h"
#include "xl9555.h"
#include "nvs_flash.h"
#include "led.h"

void show_message(void)
{
    printf("\n");
    printf("*************************************\n");
    printf("ESP32-S3\n");
    printf("EXIO TEST\n");
    printf("AUTHOR@LIU-LEI\n");
    printf("KEY0:Beep On, KEY1:Beep Off\n");
    printf("KEY2:Led On, KEY3:Led Off\n");
    printf("*************************************\n");
    printf("\n");
}

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
    soft_iic_t* iic_master = soft_iic_init(SOFT_IIC_SCL_PIN, SOFT_IIC_SDA_PIN, SOFT_IIC_FREQ);
    if (iic_master == NULL) {
        ESP_LOGE("main", "soft_iic_init failed");
        return;
    }

    if (soft_iic_check_device(iic_master, XL9555_ADDR) == ESP_OK)
    {
        ESP_LOGI("main", "found device addr: %#X", XL9555_ADDR);
    } else {
        ESP_LOGE("main", "XL9555 device not found!");
        // 可根据需要选择 return 或继续
    }
    xl9555_init(iic_master);
    show_message();

    while (1)
    {
        vTaskDelay(200);
    }

    soft_iic_deinit(iic_master);
}