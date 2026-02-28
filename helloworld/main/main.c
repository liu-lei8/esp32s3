#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "24cxx.h"
#include "led.h"
#include "iic.h"
#include "xl9555.h"

i2c_obj_t i2c0_master;

const uint8_t g_text_buf[] = "ESP32-S3 EEPROM";
#define TEXT_SIZE   sizeof(g_text_buf)

void show_mesg(void)
{
    /*串口输出实验信息*/
    printf("\n");
    printf("*********************************\n");
    printf("esp32s3\n");
    printf("IIC EEPROM TEST\n");
    printf("GOOD@LIU-LEI\n");
    printf("KEY0: Write Data, KEY1: Read Data\n");
    printf("*********************************\n");
    printf("\n");
}

void app_main(void)
{
    uint8_t key;
    uint8_t datatemp[TEXT_SIZE];
    uint16_t i = 0;

    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    xl9555_init(i2c0_master);
    gpio_intr_disable(XL9555_INT_IO);
    at24cxx_init(i2c0_master);
    show_mesg();
    esp_err_t err = at24cxx_check();
    if (err != ESP_OK)
    {
        while (1)
        {
            printf("at24cxx check failed, please check!\n");
            vTaskDelay(500);
            LED_TOGGLE();
        }
    }
    printf("24C32 Ready!\n\n");

    while (1)
    {
        key = xl9555_key_scan(0);
        switch (key)
        {
            case KEY0_PRES:
            {
                at24cxx_write(0, (uint8_t*)g_text_buf, TEXT_SIZE);
                printf("The data written is %s\n", g_text_buf);
                break;
            }
            case KEY1_PRES:
            {
                at24cxx_read(0, datatemp, TEXT_SIZE);
                printf("The data read is %s\n", datatemp);
                break;
            }
            default: break;
        }

        i++;
        if (i == 20)
        {
            LED_TOGGLE();
            i = 0;
        }
        vTaskDelay(10);
    }
}