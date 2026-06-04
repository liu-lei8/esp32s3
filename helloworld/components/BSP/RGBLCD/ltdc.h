#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ltdcfont.h"
#include "xl9555.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include <math.h>


/*RGBLCD_BL背光板*/
#define LCD_BL(x)   do{x ? gpio_set_level(GPIO_LCD_BL, 1) : \
                           gpio_set_level(GPIO_LCD_BL, 0); \
                    }while(0)

/*RGBLCD引脚*/
#define GPIO_LCD_DE     GPIO_NUM_4
#define GPIO_LCD_HSYNC  GPIO_NUM_NC
#define GPIO_LCD_VSYNC  GPIO_NUM_NC
#define GPIO_LCD_PCLK   GPIO_NUM_5
#define GPIO_LCD_BL     GPIO_NUM_42     /*背光板引脚*/

#define GPIO_LCD_R3     GPIO_NUM_45
#define GPIO_LCD_R4     GPIO_NUM_48
#define GPIO_LCD_R5     GPIO_NUM_47
#define GPIO_LCD_R6     GPIO_NUM_21
#define GPIO_LCD_R7     GPIO_NUM_14

#define GPIO_LCD_G2     GPIO_NUM_10
#define GPIO_LCD_G3     GPIO_NUM_9
#define GPIO_LCD_G4     GPIO_NUM_46
#define GPIO_LCD_G5     GPIO_NUM_3
#define GPIO_LCD_G6     GPIO_NUM_8
#define GPIO_LCD_G7     GPIO_NUM_18

#define GPIO_LCD_B3     GPIO_NUM_17
#define GPIO_LCD_B4     GPIO_NUM_16
#define GPIO_LCD_B5     GPIO_NUM_15
#define GPIO_LCD_B6     GPIO_NUM_7
#define GPIO_LCD_B7     GPIO_NUM_6

/*常用颜色值*/
#define WHITE               0xFFFF/*白色*/
#define BLACK               0x0000/*黑色*/
#define RED                 0xF800/*红色*/
#define GREEN               0x07E0/*绿色*/
#define BLUE                0x001F/*蓝色*/
#define MAGENTA             0XF81F/*品红色/紫红色= BLUE + RED */
#define YELLOW              0XFFE0/*黄色= GREEN + RED */
#define CYAN                0X07FF/*青色= GREEN + BLUE */

/*LTDC重要参数集*/
typedef struct
{
    /*LTDC面板的宽度，固定参数，不随显示方向改变，如果为0，说明没有任何RGB屏接入*/
    uint32_t pwidth;
    uint32_t pheight;   /*LTDC面板的高度，固定参数，不随显示方向改变*/
    uint16_t hsw;       /*水平同步宽度*/
    uint16_t vsw;       /*垂直同步宽度*/
    uint16_t hbp;       /*水平后廊*/
    uint16_t hfp;       /*水平前廊*/
    uint16_t vbp;       /*垂直后廊*/
    uint16_t vfp;       /*垂直前廊*/
    uint8_t activelayer;/*当前层编号：0/1*/
    uint8_t dir;        /*0：竖屏，1：横屏*/
    uint16_t id;        /*LTDC ID*/
    uint32_t pclk_hz;   /*设置像素时钟*/
    uint16_t width;     /*LTDC宽度*/
    uint16_t height;    /*LTDC高度*/
}_ltdc_dev;

extern _ltdc_dev ltdcdev;
extern esp_lcd_panel_handle_t panel_handle;

uint16_t ltdc_panelid_read(void);
void ltdc_init(void);
void ltdc_clear(uint16_t color);
uint16_t ltdc_rgb888_to_565(uint8_t r, uint8_t g, uint8_t b);
void ltdc_display_dir(uint8_t dir);
void ltdc_draw_point(uint16_t x, uint16_t y, uint16_t color);
void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);
void ltdc_draw_line(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);
void ltdc_draw_rectangle(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);
void ltdc_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void ltdc_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint16_t fc, uint16_t bc, uint8_t mode);
void ltdc_show_string(uint16_t x, uint16_t y, const char* str, uint8_t size, uint16_t fc,uint16_t bc, uint8_t mode);
/**
 * @brief 显示数值到ltdc上，最高显示无符号32位二进制值
 * @param num:要显示的十进制数值
 * @param len:要显示最高十进制的位数
 * @param size:字体大小
 * @param mode:1：叠加模式，0：非叠加模式
 * @return 返回实际有效的十进制位数的个数
 */
uint8_t ltdc_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t fc, uint16_t bc, uint8_t mode);
/**
 * @brief 显示单色图标
 * @param iconbase：指向图标点阵集的指针
 * @param width：图标的宽度
 * @param height：图标的高度
 * @note 图标的点阵集取模方式：阴码+逐行式+顺向+C51格式
 */
void ltdc_show_mono_icon(uint16_t x, uint16_t y, uint8_t* iconbase, uint16_t width, uint16_t height, uint16_t fc, uint16_t bc);