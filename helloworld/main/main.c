#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "adc.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    uint16_t adc_data;
    float adc_vol;
    char buf[10];

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    spi2_init();
    xl9555_init(i2c0_master);
    lcd_init();
    adc_init();

    lcd_show_string(10, 50, "ESP32-S3", RED, WHITE, 32, 1);
    lcd_show_string(10, 90, "ADC TEST", RED, WHITE, 24, 1);
    lcd_show_string(10, 120, "AUTHOR@LIU-LEI", RED, WHITE, 16, 1);
    lcd_show_string(10, 140, "ADC_VAL:", BLUE, WHITE, 16, 1);
    lcd_show_string(10, 160, "ADC_VOL:", BLUE, WHITE, 16, 1);

    while(1)
    {
        adc_data = adc_get_result_average(ADC_CHAN, 10);    /*获取AD值*/
        sprintf(buf, "%04d", adc_data);
        lcd_show_string(74, 140, buf, BLUE, WHITE, 16, 0);

        adc_vol = adc_data * 3.3 / 4095;
        adc_data = (uint16_t)adc_vol;                       /*获取电压取整部分*/
        sprintf(buf, "%01d.", adc_data);
        lcd_show_string(74, 160, buf, BLUE, WHITE, 16, 0);

        adc_vol -= adc_data;
        adc_vol *= 1000;
        sprintf(buf, "%03d", (uint16_t)adc_vol);              /*获取电压小数部分*/
        lcd_show_string(90, 160, buf, BLUE, WHITE, 16, 0);

        LED_TOGGLE();
        vTaskDelay(500);
    }

}