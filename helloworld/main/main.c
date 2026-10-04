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
#include "lwip_tcpserver.h"

/* FreeRTOS配置 */
/* LED_TASK配置
 * 包括：任务句柄，任务优先级，堆栈大小，创建任务
 */
#define LED_TASK_PRIO               10
#define LED_STK_SIZE                2048
TaskHandle_t LEDTask_Handler;
void led_task(void* arg);

/* KEY_TASK配置
 * 包括：任务句柄，任务优先级，堆栈大小，创建任务
 */
#define KEY_TASK_PRIO               11
#define KEY_STK_SIZE                2048
TaskHandle_t KEYTask_Handler;
void key_task(void* arg);

static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED; /* 自旋锁 */

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
    lcd_show_string(30, 70, "WIFI UDP TEST", RED, WHITE, 16, 1);
    lcd_show_string(30, 90, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);

    wifi_sta_init();

    taskENTER_CRITICAL(&my_spinlock);

    /* key任务 */
    xTaskCreatePinnedToCore(key_task, "key_task", KEY_STK_SIZE, NULL, KEY_TASK_PRIO, KEYTask_Handler, 1);

    /* led任务 */
    xTaskCreatePinnedToCore(led_task, "led_task", LED_STK_SIZE, NULL, LED_TASK_PRIO, LEDTask_Handler, 1);

    taskEXIT_CRITICAL(&my_spinlock);

    /* lwip测试代码 */
    lwip_tcpserver_demo();
}


/**
 * @brief       key_task
 * @param       arg : 传入参数(未用到)
 * @retval      无
 */
void key_task(void* arg)
{
    arg = arg;

    uint8_t key;

    while (1)
    {
        key = xl9555_key_scan(0);

        if (key == KEY0_PRES)
        {
            g_lwip_send_flag2 |= LWIP_SEND_DATA;     /* 标记LWIP有数据发送 */
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief       led_task
 * @param       arg : 传入参数(未用到)
 * @retval      无
 */
void led_task(void* arg)
{
    arg = arg;

    while (1)
    {
        LED_TOGGLE();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

