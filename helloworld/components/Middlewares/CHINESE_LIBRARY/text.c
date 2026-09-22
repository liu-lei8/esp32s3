#include "text.h"

/**
 * @brief 通过GBK编码获取字库里的点阵数据
 * @param code:GBK编码指针
 * @param matrix:点阵数据指针
 * @param size:要显示的字体大小
 */
static void text_get_hz_matrix(uint8_t* code, uint8_t* matrix, uint8_t size)
{
    uint8_t csize;
    uint8_t qh, ql;
    uint32_t foffset;

    csize = ((size / 8) + ((size % 8) ? 1 : 0)) * size; /*计算一个汉字字符的点阵字节数*/
    qh = *code;
    ql = *(++code);
    if (qh < 0x81 || ql < 0x40 || ql == 0x7F || ql == 0xFF || qh == 0xFF)   /*非常用汉字*/
    {
        for (uint8_t i = 0; i < csize; i++)
        {
            *matrix++ = 0x00;       /*填充空白*/
        }
        return;
    }

    if (ql < 0x7F)
    {
        ql -= 0x40;
    }
    else
    {
        ql -= 0x41;
    }

    qh -= 0x81;
    foffset = (190 * qh + ql) * csize;      /*得到字库中字节偏移量*/

    switch (size)
    {
    case 12:
        fonts_partition_read(ftinfo.f12addr + foffset, matrix, csize);
        break;
    case 16:
        fonts_partition_read(ftinfo.f16addr + foffset, matrix, csize);
        break;
    case 24:
        fonts_partition_read(ftinfo.f24addr + foffset, matrix, csize);
        break;
    default:
        break;
    }
}

void text_show_font(uint16_t x, uint16_t y, uint8_t* font, uint8_t size, uint8_t mode,uint16_t fc, uint16_t bc)
{
    uint8_t csize;
    uint8_t* dzk;
    uint8_t temp;
    uint8_t x1 = 0, y1 = 0;
    uint8_t y0 = y1;
    uint16_t buf[size][size];
    memset(buf, 0, sizeof(buf));

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * size;   /*一个汉字点阵数据字节数*/
    if (size != 12 && size != 16 && size != 24)
    {
        ESP_LOGE("汉字显示", "Chinese Character size not match");
        return;
    }

    dzk = malloc(size * 5); /*给存储点阵数据的指针分配内存*/
    if (dzk == NULL)
    {
        ESP_LOGE("汉字显示", "Failed to alloc memory for dzk");
        return;
    }

    text_get_hz_matrix(font, dzk, size);    /*获取点阵数据*/


    for (uint8_t i = 0; i < csize; i++)     /*填充二维缓冲区*/
    {
        temp = dzk[i];  /*获取点阵数据*/
        for (uint8_t j = 0; j < 8; j++)
        {
            if (temp & 0x80)
            {
                buf[x1][y1] = fc;   /*填充字体颜色 */
            }
            else if (mode == 0)
            {
                buf[x1][y1] = bc;   /*填充背景颜色*/
            }
            temp <<= 1;
            y1++;
            if (y1 - y0 == size)
            {
                y1 = y0;
                x1++;
                break;
            }
        }
    }


    if (!mode)                                          /*非叠加模式*/
    {
        lcd_address_set(x, x + size - 1, y, y + size - 1);  /*设置显示区域*/
        for (uint8_t i = 0; i < size; i++)
        {
            for (uint8_t j = 0; j < size; j++)
            {
                lcd_write_data16(buf[j][i]);
            }
        }
    }
    else                                                /*叠加模式*/
    {
        for (uint8_t i = 0; i < size; i++)
        {
            for (uint8_t j = 0; j < size; j++)
            {
                if (buf[j][i] == fc)
                {
                    lcd_draw_point(x + j, y + i, fc);
                }
            }
        }
    }

    free(dzk);

}

/**
 * @brief 通过字库在lcd上显示字符串(只接受GBK编码字符串)
 *  @note:可以显示中英文，并且支持自动换行和手动换行
 * @param xy:lcd上要显示的起始坐标
 * @param width:指定显示区域的宽度
 * @param height:指定显示区域的高度
 * @param str:要显示的GBK编码字符串
 * @param size:要显示的字体大小
 * @param mode:1,叠加模式；0，非叠加模式
 * @param fc:字体颜色
 * @param bc:字体背景颜色
 */
static void text_show_string_gbk(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t* str, uint8_t size, uint8_t mode, uint16_t fc, uint16_t bc)
{
    uint16_t x0 = x;
    uint16_t y0 = y;
    uint8_t bHz = 0;                /*字符或者中文*/
    uint8_t* pstr = str;  /*指向char*型字符串首地址*/


    while (*pstr != 0)              /*数据未结束*/
    {
        if (!bHz)                   /*英文字符*/
        {
            if (*pstr > 0x80)
            {
                bHz = 1;        /*标记是中文*/
            }
            else                /*字符*/
            {
                if (x > (x0 + width - size / 2))        /*换行*/
                {
                    x = x0;
                    y += size;
                }

                if (y > (y0 + height - size))           /*越界*/
                {
                    break;
                }

                if (*pstr == 10)         /*换行符*/
                {
                    x = x0;
                    y += size;
                }
                else
                {
                    lcd_show_char(x, y, *pstr, fc, bc, size, mode); /*打印字符*/
                }

                pstr++;
                x += size / 2;                          /*英文字符宽度为中文字符宽度的一半*/
            }
        }
        else                        /*中文*/
        {
            bHz = 0;                /*归位*/

            if (x > (x0 + width - size))    /*换行*/
            {
                x = x0;
                y += size;
            }

            if (y > (y0 + height - size))   /*越界*/
            {
                break;
            }

            if (*pstr == 10 || *pstr == 13)                /*遇到换行或回车符，则换行*/
            {
                x = x0;
                y += size;
            }
            else
            {
                text_show_font(x, y, pstr, size, mode, fc, bc);
            }

            pstr += 2;
            x += size;
        }
    }

}
    
void text_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char* str, uint8_t size, uint8_t mode, uint16_t fc, uint16_t bc)
{
    int len = strlen(str) * 2 + 1;
    uint8_t* gbk_buf = malloc(len);
    if (!gbk_buf) return;

    if (utf8_to_gbk(str, gbk_buf, len) > 0)
    {
        text_show_string_gbk(x, y, width, height, gbk_buf, size, mode, fc, bc);
    }

    free(gbk_buf);
}

void text_show_string_middle(uint16_t x, uint16_t y, uint16_t width, char* str, uint8_t size, uint8_t mode, uint16_t fc ,uint16_t bc)
{
    uint16_t strlenght = 0;
    strlenght = strlen(str);
    strlenght *= size / 2;      /*点阵宽度*/

    if (strlenght >= width)
    {
        text_show_string(x, y, LCD_W - x, LCD_H - y, str, size, mode, fc, bc);
    }
    else
    {
        strlenght = (width - strlenght) / 2;
        text_show_string(x + strlenght, y, LCD_W, LCD_H, str, size, mode ,fc, bc);
    }
}

