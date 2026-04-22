#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "rmt_nec_rx.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    rmt_rx_done_event_data_t rx_data;

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    spi2_init();
    xl9555_init(i2c0_master);
    lcd_init();
    rmt_nec_rx_init();

    lcd_show_string(30, 50, "ESP32-S3", RED, WHITE, 32, 1);
    lcd_show_string(30, 90, "RMT RX TEST", RED, WHITE, 24, 1);
    lcd_show_string(30, 120, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);
    lcd_show_string(30, 140, "KEYVAL:", RED, WHITE, 16, 1);
    lcd_show_string(30, 160, "SYMBOL:", RED, WHITE, 16, 1);

    while (1)
    {
        if (xQueueReceive(receive_queue, &rx_data, pdMS_TO_TICKS(1000)) == pdPASS)
        {
            rmt_rx_scan(rx_data.received_symbols, rx_data.num_symbols); /*解析接收符号并打印结果*/
            ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config));    /*重新开始接收,驱动层目前不支持直接配置为连续接收模式*/
        }
        LED_TOGGLE();
        vTaskDelay(10);
    }
    
}