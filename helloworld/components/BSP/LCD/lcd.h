#pragma once
/*使用的浦洋的1.69寸lcd*/
#include "driver/gpio.h"
#include "xl9555.h"
#include "spi.h"
#include "lcdfont.h"

#define LCD_NUM_WR      GPIO_NUM_40
#define LCD_NUM_CS      GPIO_NUM_21     /*因为spi从机只有lcd，所以这里将片选直接接地，省下一个引脚*/

#define LCD_WR(x)       do{ x ? \
                            gpio_set_level(LCD_NUM_WR, 1):  \
                            gpio_set_level(LCD_NUM_WR, 0);  \
                        }while(0)       /*1写数据，0写命令*/

#define LCD_CS(x)       do{ x ? \
                            gpio_set_level(LCD_NUM_CS, 1):  \
                            gpio_set_level(LCD_NUM_CS, 0);  \
                        }while(0)       /*片选，拉低电平选中*/

#define LCD_PWR(x)      do{ x ? \
                            xl9555_pin_write(SLCD_PWR_IO, 1):   \
                            xl9555_pin_write(SLCD_PWR_IO, 0);   \
                        }while(0)       /*1打开背光板，0关闭背光板*/

#define LCD_RST(x)      do{ x ? \
                            xl9555_pin_write(SLCD_RST_IO, 1):   \
                            xl9555_pin_write(SLCD_RST_IO, 0);   \
                        }while(0)       /*拉低电平复位*/

/*常用颜色值*/
#define WHITE               0xFFFF/*白色*/
#define BLACK               0x0000/*黑色*/
#define RED                 0xF800/*红色*/
#define GREEN               0x07E0/*绿色*/
#define BLUE                0x001F/*蓝色*/
#define MAGENTA             0XF81F/*品红色/紫红色= BLUE + RED */
#define YELLOW              0XFFE0/*黄色= GREEN + RED */
#define CYAN                0X07FF/*青色= GREEN + BLUE */
/*非常用颜色*/
#define BROWN               0XBC40/*棕色*/
#define BRRED               0XFC07/*棕红色*/
#define GRAY                0X8430/*灰色*/
#define DARKBLUE            0X01CF/*深蓝色*/
#define LIGHTBLUE           0X7D7C/*浅蓝色*/
#define GRAYBLUE            0X5458/*灰蓝色*/
#define LIGHTGREEN          0X841F/*浅绿色*/
#define LGRAY               0XC618/*浅灰色(PANNEL),窗体背景色*/
#define LGRAYBLUE           0XA651/*浅灰蓝色(中间层颜色) */
#define LBBLUE              0X2B12/*浅棕蓝色(选择条目的反色) */

/*扫描方向定义*/
#define L2R_U2D         0           /*从左到右，从上到下*/
#define L2R_D2U         1           /*从左到右，从下到上*/
#define R2L_U2D         2           /*从右到左，从上到下*/
#define R2L_D2U         3           /*从右到左，从下到上*/
#define U2D_L2R         4           /*从上到下，从左到右*/
#define U2D_R2L         5           /*从上到下，从右到左*/
#define D2U_L2R         6           /*从下到上，从左到右*/
#define D2U_R2L         7           /*从下到上，从右到左*/

#define DEF_SCAN_DIR    L2R_U2D     /*默认扫描方向*/

typedef struct{
    uint16_t width;     /*宽度*/
    uint16_t height;    /*高度*/
    uint8_t  dir;       /*0，竖屏；1，横屏*/
    uint16_t wramcmd;   /*开始写gram指令*/
    uint16_t setxcmd;   /*设置x坐标指令*/
    uint16_t setycmd;   /*设置y坐标指令*/
    uint16_t wr;        /*命令/数据IO*/
    uint16_t cs;        /*片选IO*/
}lcd_obj_t; /*没有用上*/

#define LCD_TOTAL_PIXEL_SIZE    240 * 280           /*67200个像素点*/
#define LCD_TOTAL_BUF_SIZE      240 * 280 * 2

#define USE_HORIZONTAL 0    /*设置横屏和竖屏显示 0或1为竖屏，2或3为横屏*/

#if USE_HORIZONTAL == 0 || USE_HORIZONTAL == 1
#define LCD_W 240
#define LCD_H 280
#else
#define LCD_W 280
#define LCD_H 240
#endif

#define SCREEN_WIDTH    LCD_W
#define SCREEN_HEIGHT   LCD_H

void lcd_init(void);

void lcd_write_cmd(const uint8_t cmd);

/*********************************************************************
*函数说明：连续写入像素数据，并对颜色值高低字节进行了转换
*入口函数：data      RGB565的颜色数据
          len       像素个数
*返回值：无
***********************************************************************/
void lcd_write_data(uint16_t* data, int len);

void lcd_write_data8(uint8_t data);

void lcd_write_data16(uint16_t data);

void lcd_hard_reset(void);

void lcd_initialize(void);

void lcd_address_set(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2);

void lcd_fill(uint16_t xstart, uint16_t xend, uint16_t ystart, uint16_t yend, uint16_t color);

void lcd_clear(uint16_t color);

void lcd_draw_poiont(uint16_t x, uint16_t y, uint16_t color);

/*********************************************************************
*函数说明：显示一个字符
*入口函数：fc       字符颜色
          bc       字符的背景颜色
          sizey    12、16、24、32字体大小
          mode     1叠加模式（只用字体颜色覆盖）0非叠加模式（字体和背景颜色覆盖）
*返回值：无
***********************************************************************/
void lcd_show_char(uint16_t x, uint16_t y, uint8_t chr, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);

void lcd_show_string(uint16_t x, uint16_t y, const char* string, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);

/*lcd模块没有MISO的引脚，所以读的值没用*/
uint8_t lcd_read_id(void);

/*该函数只能使用32X32的字体大小*/
void lcd_show_chinese(uint16_t x, uint16_t y, const char* chinese, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);

void lcd_show_chinese_string(uint16_t x, uint16_t y, const char* chinese, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);

/*********************************************************************
*函数说明：显示240X280的图片
*入口函数：picture      选择图片模板
          mode         1叠加模式、0非叠加模式
*返回值：无
***********************************************************************/
void lcd_show_picture(uint8_t picture, uint16_t fc, uint16_t bc, uint8_t mode);

/*该函数只能叠加模式。查找像素模板的逻辑不同于上面函数而已，功能一样*/
void lcd_show_picture0(uint8_t picture, uint16_t fc, uint16_t bc);
