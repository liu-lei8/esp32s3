#pragma once /*text.h 有关汉字显示代码*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fonts.h"
#include "unicode_gbk.h"

/**
 * @brief 通过字库在lcd上显示单个汉字
 * @param xy:lcd上要显示的起始坐标
 * @param font:GBK编码指针
 * @param size:要显示的字体大小
 * @param mode:1,叠加模式；0，非叠加模式
 * @param fc:字体颜色
 * @param bc:字体背景颜色
 */
void text_show_font(uint16_t x, uint16_t y, uint8_t* font, uint8_t size, uint8_t mode,uint16_t fc, uint16_t bc);

/**
 * @brief 通过字库在lcd上显示字符串
 *  @note:可以显示中英文，并且支持自动换行和手动换行
 * @param xy:lcd上要显示的起始坐标
 * @param width:指定显示区域的宽度
 * @param height:指定显示区域的高度
 * @param str:要显示的UTF-8编码字符串
 * @param size:要显示的字体大小
 * @param mode:1,叠加模式；0，非叠加模式
 * @param fc:字体颜色
 * @param bc:字体背景颜色
 */
void text_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char* str, uint8_t size, uint8_t mode, uint16_t fc, uint16_t bc);

/**
 * @brief 通过字库在lcd上指定宽度上居中显示字符串
 *  @note:可以显示中英文，并且支持自动换行和手动换行
 * @param xy:lcd上要显示的起始坐标
 * @param width:指定显示区域的宽度
 * @param str:要显示的字符串
 * @param size:要显示的字体大小
 * @param mode:1,叠加模式；0，非叠加模式
 * @param fc:字体颜色
 * @param bc:字体背景颜色
 */
void text_show_string_middle(uint16_t x, uint16_t y, uint16_t width, char* str, uint8_t size, uint8_t mode, uint16_t fc ,uint16_t bc);