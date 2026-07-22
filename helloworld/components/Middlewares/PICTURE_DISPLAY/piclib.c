#include "piclib.h"

_pic_info picinfo;      /*图片信息*/
_pic_phy pic_phy;       /*图片显示的物理接口*/

/**
 * @brief 多点填充，即填充一行的像素
 * @param xy:起始坐标
 * @param width:要填充的宽度
 * @param color:颜色数组
 */
static void piclib_multi_color(uint16_t x, uint16_t y, uint16_t width, uint16_t* color)
{
    lcd_address_set(x, x + width - 1, y, y);
    lcd_write_data(color, width);
}


void piclib_init(void)
{
    pic_phy.draw_point = lcd_draw_poiont;       /*画点函数实现，仅GIF需要 */
    pic_phy.fill = lcd_fill;                    /*填充函数实现，仅GIF需要 */
    pic_phy.draw_hline = lcd_draw_hline;        /*画线函数实现，仅GIF需要 */
    pic_phy.multicolor = piclib_multi_color;    /*颜色填充函数实现，JPEG,BMP,PNG需要 */

    picinfo.lcdwidth = lcd_self.width;          /*得到LCD宽度像素*/
    picinfo.lcdheight = lcd_self.height;        /*得到LCD高度像素*/
}

uint32_t piclib_ai_load_picfile(char* filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    uint32_t res = 0;       /*返回值*/
    uint8_t temp;

    if ((x + width - 1)  > picinfo.lcdwidth) return PIC_WINDOW_ERR;     /*x坐标超范围了*/
    if ((y + height - 1) > picinfo.lcdheight) return PIC_WINDOW_ERR;    /*y坐标超范围了*/

    if (width == 0 || height == 0) return PIC_WINDOW_ERR;   /*窗口设定错误*/

    temp = exfuns_file_type(filename);
    ESP_LOGI("piclib", "File type:%#x", temp);

    switch (temp)
    {
        ESP_LOGI("piclib", "Enter picture decode!");
        case T_BMP:
            res = bmp_decode(filename, width, height);          /*解码BMP*/
            break;
        case T_JPG:
        case T_JPEG:
            res = jpeg_decode(filename, width, height);         /*解码JPG/JPEG */
            break;
        case T_GIF:
            res = gif_decode(filename, x, y, width, height);    /*解码GIF*/
            break;
        case T_PNG:
            res = png_decode(filename, width,height);           /*解码PNG*/
            break;
        default:
            res = PIC_FORMAT_ERR;                               /*非图片格式*/
            break;
    }

    return res;
}

void* piclib_mem_malloc(uint32_t size)
{
    return (void*)malloc(size);
}

void piclib_mem_free(void* paddr)
{
    free(paddr);
}
