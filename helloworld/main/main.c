#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "spi_sd.h"
#include "key.h"
#include "nvs_flash.h"
#include "my_jpeg_decode.h"

i2c_obj_t i2c0_master;
extern uint8_t sd_check_en;
extern SemaphoreHandle_t BinarySemaphore;
extern TaskHandle_t task3_handler;
extern camera_fb_t* fb;
extern sdmmc_card_t* card;


void app_main(void)
{
    esp_err_t ret;
    uint8_t key = 0;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    spi2_init();
    xl9555_init(i2c0_master);
    lcd_init();
    camera_init();

    lcd_show_string(30, 30, "ESP32-S3", RED ,WHITE, 32, 1);
    lcd_show_string(30, 70, "TAKE PHOTE TEST", RED ,WHITE, 24, 1);
    lcd_show_string(30, 100, "AUTHOR@LIU-LEI",RED, WHITE, 16, 1);

    while(sd_spi_init())
    {
        lcd_show_string(30, 120, "SD Card Error!", RED, WHITE, 16, 0);
        lcd_show_string(30, 140, "Please Check!", RED, WHITE, 16, 0);
        vTaskDelay(500);
    }
    sd_check_en = 1;

    uint8_t* rgb565 = (uint8_t*)malloc(LCD_W * LCD_H * 2);
    if (!rgb565)
    {
        ESP_LOGE("main", "Can't allocate memory for rgb565 buffer");
    }

    BinarySemaphore = xSemaphoreCreateBinary();
    /*创建任务3*/
    xTaskCreatePinnedToCore((TaskFunction_t  )task3,                  /*任务函数*/
                            (const char*     )"task3",                /*任务名称*/
                            (const uint32_t  )TASK3_STK_SIZE,         /*任务堆栈大小*/
                            (void*           )NULL,                   /*传给任务函数的参数*/
                            (UBaseType_t     )TASK3_PRIORITY,         /*任务优先级*/
                            (TaskHandle_t*   )&task3_handler,         /*任务句柄*/
                            (const BaseType_t)0                   /*该任务在哪个内核运行*/);

    vTaskDelay(pdMS_TO_TICKS(1500));

    while(1)
    {
        key = key_scan(0);
        fb = esp_camera_fb_get();       /*这里摄像头配置使用的是JPEG模式*/

        if (fb)
        {
            decode_jpeg(fb->buf, fb->len, rgb565);  /*将JPEG格式数据转换为RGB565格式数据*/

            if (key == BOOT_PRES)
            {
                xSemaphoreGive(BinarySemaphore);        /*释放二值信号量*/
            }

            /*处理SD卡热插拔*/
            if (sd_check_en == 1)
            {
                if (sdmmc_get_status(card) != ESP_OK)
                {
                    sd_check_en = 0;
                }
            }
            else
            {
                if (sd_spi_init() == ESP_OK)
                {
                    if (sdmmc_get_status(card) == ESP_OK)
                    {
                        sd_check_en = 1;
                    }
                }
            }

            camera_show(0, 0, rgb565);
        }
        else
        {
            ESP_LOGE("main", "Get frame failed!");
        }
        esp_camera_fb_return(fb);

        vTaskDelay(15);
        LED_TOGGLE();
    }
}