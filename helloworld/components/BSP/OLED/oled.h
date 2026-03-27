#pragma once
/*使用的是SSD1315四脚的oled模块*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iic.h"
#include <string.h>

#define OLED_ADDR       0X3C        /*7位的器件地址*/
#define OLED_CMD        0X00        /*写命令*/
#define OLED_DATA       0X40        /*写数据*/

void oled_init(i2c_obj_t self);
void oled_on(void);
void oled_off(void);
void oled_clear(void);
void oled_reflash_gram(void);
void oled_write_Byte(unsigned char tx_data, unsigned char command);
esp_err_t oled_write(uint8_t* data_wr, size_t size);
void oled_draw_point(uint8_t x, uint8_t y, uint8_t dot);
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t mode);
void oled_show_string(uint8_t x, uint8_t y, char* string, uint8_t size);
void oled_show_num(uint8_t x, uint8_t y, uint8_t chr, uint8_t size);

