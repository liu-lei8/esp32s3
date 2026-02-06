#pragma once

#include "iic.h"
#include "esp_log.h"
#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define XL9555_INT_IO       GPIO_NUM_40
#define XL9555_INT          gpio_get_level(XL9555_INT_IO)
#define XL9555_ADDR         0X20

#define XL9555_INPUT_PORT0_REG      0x00
#define XL9555_INPUT_PORT1_REG      0x01
#define XL9555_OUTPUT_PORT0_REG     0x02
#define XL9555_OUTPUT_PORT1_REG     0x03
#define XL9555_INVERSION_PORT0_REG  0x04
#define XL9555_INVERSION_PORT1_REG  0x05
#define XL9555_CONFIG_PORT0_REG     0x06
#define XL9555_CONFIG_PORT1_REG     0x07

/*XL9555各个IO的功能*/
#define AP_INT_IO        0x0001  /* AP3216C中断引脚P00 */
#define QMA_INT_IO       0x0002  /* QMA6100P中断引脚P01 */
#define SPK_EN_IO        0x0004  /*功放使能引脚P02 */
#define BEEP_IO          0x0008  /*蜂鸣器控制引脚P03 */
#define OV_PWDN_IO       0x0010  /*摄像头待机引脚P04 */
#define OV_RESET_IO      0x0020  /*摄像头复位引脚P05 */
#define GBC_LED_IO       0x0040  /* ATK_MODULE接口LED引脚P06 */
#define GBC_KEY_IO       0x0080  /* ATK_MODULE接口KEY引脚P07 */
#define LCD_BL_IO        0x0100  /* RGB屏背光控制引脚P10 */
#define CT_RST_IO        0x0200  /*触摸屏中断引脚P11 */
#define SLCD_RST_IO      0x0400  /*SPI_LCD复位引脚P12 */
#define SLCD_PWR_IO      0x0800  /* SPI_LCD控制背光引脚P13 */
#define KEY3_IO          0x1000  /*按键3引脚P14 */
#define KEY2_IO          0x2000  /*按键2引脚P15 */
#define KEY1_IO          0x4000  /*按键1引脚P16 */
#define KEY0_IO          0x8000  /*按键0引脚P17 */

#define KEY0    xl9555_pin_read(KEY0_IO)
#define KEY1    xl9555_pin_read(KEY1_IO)
#define KEY2    xl9555_pin_read(KEY2_IO)
#define KEY3    xl9555_pin_read(KEY3_IO)

#define KEY0_PRES   1
#define KEY1_PRES   2
#define KEY2_PRES   3
#define KEY3_PRES   4

void xl9555_init(i2c_obj_t self);
esp_err_t xl9555_write_byte(uint8_t reg, uint8_t* data, size_t len);
esp_err_t xl9555_pin_write(uint16_t xl9555_pin, bool level);
esp_err_t xl9555_read_byte(uint8_t* data, size_t len);
bool xl9555_pin_read(uint16_t xl9555_pin);
uint16_t xl9555_ioconfig(uint16_t ioconfig);
uint8_t xl9555_key_scan(bool mode);