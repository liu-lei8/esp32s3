#include "iic.h"
#include "xl9555.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "led.h"

void show_message(void)
{
    /*串口输出实验信息*/
    printf("\n");
    printf("********************************\n");
    printf("ESP32-S3\n");
    printf("EXIOTEST\n");
    printf("ATOM@ALIENTEK\n");
    printf("KEY0:Beep On,KEY1:Beep Off\n");
    printf("KEY2:LED On,KEY3:LED Off\n");
    printf("********************************\n");
    printf("\n");
}

static bool i2c_probe(i2c_obj_t* i2c, uint8_t addr)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c->port, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);

    return ret == ESP_OK;
}

static void i2c_scan(i2c_obj_t* i2c)
{
    printf("I2C scanning...\n");
    for (uint8_t addr = 0x03; addr < 0x78; ++addr)
    {
        if (i2c_probe(i2c, addr))
        {
            printf("Found I2C device at 0x%02X\n", addr);
        }
    }
}

void app_main(void)
{
    uint8_t key;
    i2c_obj_t i2c0_master;
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    /* 扫描总线上的 I2C 设备并打印地址 */
    i2c_scan(&i2c0_master);
    xl9555_init(i2c0_master);
    show_message();

    while (1)
    {
        key = xl9555_key_scan(0);
        switch (key)
        {
            case KEY0_PRES:
            {
                printf("KEY0 has been pressed\n");
                xl9555_pin_write(BEEP_IO, 0);       /*打开蜂鸣器*/
                break;
            }
            case KEY1_PRES:
            {
                printf("KEY1 has been pressed\n");
                xl9555_pin_write(BEEP_IO, 1);       /*关闭蜂鸣器*/
                break;
            }
            case KEY2_PRES:
            {
                printf("KEY2 has been pressed\n");
                LED(0);
                break;
            }
            case KEY3_PRES:
            {
                printf("KEY3 has been pressed\n");
                LED(1);
                break;
            }
            default:
            {
                break;
            }
        }
        if (XL9555_INT == 0)
        {
            printf("XL9555_INT success!!!\n");
        }
        vTaskDelay(200);
    }
}