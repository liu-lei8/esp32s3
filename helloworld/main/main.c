#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iic.h"
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

static esp_err_t i2c_probe(i2c_obj_t* self, uint8_t addr)
{
    esp_err_t ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(self->port, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);

    return ret;
}

static void i2c_scan(i2c_obj_t* self)
{
    printf("I2C scanning...\n");
    for (uint8_t addr = 0x03; addr < 0x78; ++addr)
    {
        if (i2c_probe(self, addr) == ESP_OK)
        {
            printf("Found I2C device at %#X\n", addr);
        }
    }
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
    i2c_obj_t iic0_master = iic_init(I2C_NUM_0);
    i2c_scan(&iic0_master);
    xl9555_init(iic0_master);
    show_message();


    while (1)
    {
        vTaskDelay(200);
    }
}