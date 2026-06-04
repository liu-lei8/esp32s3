#include "ltdc.h"

static const char* TAG = "ltdc";

_ltdc_dev ltdcdev;
esp_lcd_panel_handle_t panel_handle = NULL;

/*定义portMUX_TYPE类型的自旋锁变量，用于临界区的保护*/
static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;
uint32_t g_back_color = 0xFFFF;     /*背景色*/

uint16_t ltdc_panelid_read(void)
{
    uint8_t idx = 0;

    gpio_config_t rgb7_gpio_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ull << GPIO_LCD_R7 | 1ull << GPIO_LCD_G7 | 1ull << GPIO_LCD_B7,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&rgb7_gpio_cfg);

    idx = (uint8_t)gpio_get_level(GPIO_LCD_R7);     /*读取M0*/
    idx |= (uint8_t)gpio_get_level(GPIO_LCD_G7) << 1;    /*读取M1*/
    idx |= (uint8_t)gpio_get_level(GPIO_LCD_B7) << 2;    /*读取M2*/

    switch (idx)
    {
    case 0: return 0x4342;      /*4.3寸屏，480 * 272分辨率*/
    case 4: return 0x4384;      /*4.3寸屏，800 * 480分辨率*/
    default: return 0;
    }
}

void ltdc_init(void)
{
    panel_handle = NULL;
    ltdcdev.id = ltdc_panelid_read();

    /*根据厂家手册进行配置*/
    if (ltdcdev.id == 0x4342)
    {
        ltdcdev.pwidth = 480;               /*面板宽度，单位：像素*/
        ltdcdev.pheight = 272;              /*面板高度，单位：像素*/
        ltdcdev.hsw = 1;                    /*水平同步宽度*/
        ltdcdev.vsw = 1;                    /*垂直同步宽度*/
        ltdcdev.hbp = 40;                   /*水平后廊*/
        ltdcdev.hfp = 5;                    /*水平前廊*/
        ltdcdev.vbp = 8;                    /*垂直后廊*/
        ltdcdev.vfp = 0;                    /*垂直前廊*/
        ltdcdev.pclk_hz = 9 * 1000 * 1000;  /*设置像素时钟9Mhz*/
    }
    else if (ltdcdev.id == 0x4384)
    {
        ltdcdev.pwidth = 800;               /*面板宽度，单位：像素*/
        ltdcdev.pheight = 480;              /*面板高度，单位：像素*/
        ltdcdev.hsw = 48;                    /*水平同步宽度*/
        ltdcdev.vsw = 3;                    /*垂直同步宽度*/
        ltdcdev.hbp = 88;                   /*水平后廊*/
        ltdcdev.hfp = 40;                    /*水平前廊*/
        ltdcdev.vbp = 32;                    /*垂直后廊*/
        ltdcdev.vfp = 13;                    /*垂直前廊*/
        ltdcdev.pclk_hz = 18 * 1000 * 1000;  /*设置像素时钟18Mhz*/
    }

    esp_lcd_rgb_panel_config_t panel_config = { /*RGBLCD配置结构体*/
        .data_width = 16,                       /*数据宽度为16位*/
        .psram_trans_align = 64,                /*在PSRAM中分配的缓冲区的对齐*/
        .clk_src = LCD_CLK_SRC_PLL160M,         /*RGBLCD外设时钟源*/
        .disp_gpio_num = GPIO_NUM_NC,           /*用于显示控制信号，不使用设为-1*/
        .pclk_gpio_num = GPIO_LCD_PCLK,         /*PCLK信号引脚*/
        .hsync_gpio_num = GPIO_NUM_NC,          /*HSYNC信号引脚，DE模式可不使用*/
        .vsync_gpio_num = GPIO_NUM_NC,          /*VSYNC信号引脚，DE模式可不使用*/
        .de_gpio_num = GPIO_LCD_DE,             /*DE信号引脚*/
        .data_gpio_nums = {                     /*数据线引脚,这是蓝绿红的颜色数据顺序*/
            GPIO_LCD_B3, GPIO_LCD_B4, GPIO_LCD_B5, GPIO_LCD_B6, GPIO_LCD_B7,
            GPIO_LCD_G2, GPIO_LCD_G3, GPIO_LCD_G4, GPIO_LCD_G5, GPIO_LCD_G6, GPIO_LCD_G7,
            GPIO_LCD_R3, GPIO_LCD_R4, GPIO_LCD_R5, GPIO_LCD_R6, GPIO_LCD_R7
        },
        .timings = {                            /*RGBLCD时序参数*/
            .pclk_hz = ltdcdev.pclk_hz,         /*像素时钟频率*/
            .h_res = ltdcdev.pwidth,            /*水平分辨率，即一行中的像素数*/
            .v_res = ltdcdev.pheight,           /*垂直分辨率，即帧中的行数*/
            .hsync_back_porch = ltdcdev.hbp,
            .hsync_front_porch = ltdcdev.hfp,
            .hsync_pulse_width = ltdcdev.hsw,   /*水平同步宽度，单位：PCLK周期*/
            .vsync_back_porch = ltdcdev.vbp,
            .vsync_front_porch = ltdcdev.vfp,
            .vsync_pulse_width = ltdcdev.vsw,   /*垂直同步宽度，单位：行数*/
            .flags.pclk_active_neg = true,      /*RGB数据在下降沿计时*/
        },
        .flags.fb_in_psram = true,              /*在PSRAM中分配帧缓冲区*/
        /*解决写spiflash时，抖动问题*/
        .bounce_buffer_size_px = (ltdcdev.id == 0x4384) ? 800 * 2 : 480 * 2,
    };

    gpio_config_t lcd_bl_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ull << GPIO_LCD_BL,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&lcd_bl_cfg);

    /*创建RGB对象*/
    esp_lcd_new_rgb_panel(&panel_config, &panel_handle);
    /*复位RGB屏*/
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    /*初始化RGB屏*/
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    /*设置横屏*/
    ltdc_display_dir(1);
    /*清除屏幕为背景色*/
    ltdc_clear(WHITE);
    /*打开背光*/
    LCD_BL(1);
}

void ltdc_clear(uint16_t color)
{
    uint16_t* buffer = heap_caps_malloc(ltdcdev.width * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (buffer == NULL)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
    }
    else
    {
        for (uint16_t i = 0; i < ltdcdev.width; i++)
        {
            buffer[i] = color;
        }
        for (uint16_t j = 0; j < ltdcdev.height; j++)
        {
            taskENTER_CRITICAL(&my_spinlock);   /*屏蔽中断，保护画点过程，禁止任务调度*/
            esp_lcd_panel_draw_bitmap(panel_handle, 0, j, ltdcdev.width, j + 1, buffer);
            taskEXIT_CRITICAL(&my_spinlock);    /*重新使能中断*/
        }
        heap_caps_free(buffer);
    }
}

uint16_t ltdc_rgb888_to_565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void ltdc_display_dir(uint8_t dir)
{
    ltdcdev.dir = dir;

    if (ltdcdev.dir == 0)   /*竖屏方向*/
    {
        ltdcdev.width = ltdcdev.pheight;
        ltdcdev.height = ltdcdev.pwidth;
        esp_lcd_panel_swap_xy(panel_handle, true);          /*交换xy轴*/
        esp_lcd_panel_mirror(panel_handle, false, true);    /*对屏幕y轴进行镜像处理*/
    }
    else if (ltdcdev.dir == 1)      /*横屏*/
    {
        ltdcdev.width = ltdcdev.pwidth;
        ltdcdev.height = ltdcdev.pheight;
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, true);    /*对屏幕x和y轴进行镜像处理*/
    }
}

void ltdc_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    taskENTER_CRITICAL(&my_spinlock);
    esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + 1, y + 1, &color);
    taskEXIT_CRITICAL(&my_spinlock);
}

void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    /*确保填充区域在LCD屏幕内*/
    if (sx < 0 || sy < 0 || ex > ltdcdev.width - 1 || ey > ltdcdev.height)
    {
        return;
    }

    /*确保起始坐标小于结束坐标*/
    if (sx > ex || sy > ey)
    {
        return;
    }

    /*确保填充区域完全在LCD范围内*/
    sx = fmax(0, sx);
    sy = fmax(0, sy);
    ex = fmin(ltdcdev.width - 1, ex);
    ey = fmin(ltdcdev.height - 1, ey);

    /*开始填充颜色*/
    for (int i = sx; i <= ex; i++)
    {
        for (int j = sy; j <= ey; j++)
        {
            ltdc_draw_point(i, j, color);
        }
    }
}

void ltdc_draw_line(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    int delta_x, delta_y, xerr, yerr, distance;
    uint16_t row, col;
    int8_t incx, incy;

    delta_x = ex - sx;
    delta_y = ey - sy;
    xerr = 0; yerr = 0;
    row = sx; col = sy;

    /*设置x和y的增量方向*/
    if (sx < ex)
    {
        incx = 1;
    }
    else if (sx == ex)
    {
        incx = 0;       /*垂直线*/
    }
    else
    {
        incx = -1;
    }

    if (sy < ey)
    {
        incy = 1;
    }
    else if (sy == ey)
    {
        incy = 0;       /*水平线*/
    }
    else
    {
        incy = -1;
    }

    if (delta_x >= delta_y)
    {
        distance = delta_x;     /*选取基本增量坐标轴*/
    }
    else
    {
        distance = delta_y;
    }

    /*开始画线*/
    for (uint16_t i = 0; i <= distance + 1; i++)
    {
        ltdc_draw_point(row, col, color);
        xerr += delta_x;
        yerr += delta_y;

        if (xerr > distance)
        {
            xerr -= distance;
            row += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            col += incy;
        }
    }
    
}

void ltdc_draw_rectangle(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    ltdc_draw_line(sx, sy, ex, sy, color);
    ltdc_draw_line(sx, sy, sx, ey, color);
    ltdc_draw_line(sx, ey, ex, ey, color);
    ltdc_draw_line(ex, ey, ex, sy, color);
}

void ltdc_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;
    int di;
    a = 0;
    b = r;  /*起始位置*/
    /**
     * 这个公式相当于判断F（x，y）= x² + y² - r²的大小来确定y坐标是否向下移动一个像素，
     * di = 3 - 2 * r这个公式其实就是从起始位置开始当x向右移动一个像素，y向下移动0.5个像素，
     * 代入F（0 + 1，r - 0.5）= 1.25 - r, 然后将该值✖️4消除浮点数，再+1然后除2,就是di公式了。
     * */
    di = 3 - 2 * r;    /*小于0时说明中点在圆内，b值不变，否则说明中点在圆上或圆外b值减1*/

    while (a <= b)  /*只需要画45°-90°八分之一个圆弧，再利用8个对称点就能画出完整圆*/
    {
        /*利用圆的对称性，画8个对称点*/
        ltdc_draw_point(x0 + a, y0 + b, color);     /*第一象限，靠近y轴*/
        ltdc_draw_point(x0 + b, y0 + a, color);     /*第一象限，靠近x轴*/
        ltdc_draw_point(x0 - a, y0 + b, color);     /*第二象限，靠近y轴*/
        ltdc_draw_point(x0 - b, y0 + a, color);     /*第二象限，靠近x轴*/
        ltdc_draw_point(x0 - b, y0 - a, color);     /*第三象限，靠近x轴*/
        ltdc_draw_point(x0 - a, y0 - b, color);     /*第三象限，靠近y轴*/
        ltdc_draw_point(x0 + a, y0 - b, color);     /*第四象限，靠近y轴*/
        ltdc_draw_point(x0 + b, y0 - a, color);     /*第四象限，靠近x轴*/

        /* 使用Bresenham算法画圆 */
        if (di < 0) /*中点在圆内，y轴不变*/
        {
            /*这个公式就是4F（a + 1，b）计算后减去4F（a + 1，b - 0.5）最后除以2得来的*/
            di += 4 * a + 6;
        }
        else        /*中点在圆上或圆外，y轴减1*/
        {
            /*这个公式就是4F（a + 1，b - 1）计算后减去4F（a + 1，b - 0.5）最后除以2得来的*/
            di += 4 * (a - b) + 10;
            --b;
        }
        ++a;    /*标准的算法++a应该放在判断之后*/
    }
}

void ltdc_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint16_t fc, uint16_t bc, uint8_t mode)
{
    uint8_t csize;
    uint8_t* pfont = NULL;
    uint8_t temp;
    uint16_t y0 = y;

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); /*得到一个字符对应点阵集对应的字符数*/

    chr = chr - ' ';

    switch (size)
    {
        case 12: pfont = lcd_asc2_1206[(uint8_t)chr]; break;
        case 16: pfont = lcd_asc2_1608[(uint8_t)chr]; break;
        case 24: pfont = lcd_asc2_2412[(uint8_t)chr]; break;
        case 32: pfont = lcd_asc2_3216[(uint8_t)chr]; break;
    }

    for (uint8_t i = 0; i < csize; i++)
    {
        temp = pfont[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (temp & 0x80)
            {
                ltdc_draw_point(x, y, fc);
            }
            else if (mode == 0) /*非叠加模式*/
            {
                ltdc_draw_point(x, y, bc);
            }
            temp <<= 1;
            y++;

            if (y - y0 == size)
            {
                x++;
                y = y0;
                break;
            }
        }
    }
}

void ltdc_show_string(uint16_t x, uint16_t y, const char* str, uint8_t size, uint16_t fc,uint16_t bc, uint8_t mode)
{
    while (*str >= ' ' && *str <= '~' && *str != '\0')
    {
        ltdc_show_char(x, y , *str, size, fc, bc, mode);
        str++;
        x += size / 2;
    }
}

uint8_t ltdc_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t fc, uint16_t bc, uint8_t mode)
{
    uint8_t temp;
    uint8_t t = 0;
    uint8_t enshow = 0;

    for (uint8_t i = 0; i < len; i++)
    {
        temp = (int)(num / pow10(len - i - 1)) % 10;    /*十进制从高位到低位依次获取位值*/

        if (enshow == 0 && i < len - 1) /*关闭使能显示并且不是最后一个位值,就不显示*/
        {
            if (temp == 0)
            {
                continue;       /*高位为0不显示，直接跳过*/
            }
            else
            {
                enshow = 1;     /*高位不是0了，开始使能显示*/
            }
        }

        ltdc_show_char(x + t * (size / 2), y, temp + '0', size, fc, bc, mode);
        t++;
    }
    
    return t;   /*返回实际的显示的位数，以便于之后用于小数点的显示位置*/
}

void ltdc_show_mono_icon(uint16_t x, uint16_t y, uint8_t* iconbase, uint16_t width, uint16_t height, uint16_t fc, uint16_t bc)
{
    uint8_t rsize;
    uint8_t temp;
    uint16_t n = 0;

    uint16_t* buffer = heap_caps_malloc(width * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    rsize = width / 8 + ((width % 8) ? 1 : 0);  /*一行多少个字节*/
    for (uint16_t j = 0; j < height * rsize; j++)
    {
        temp = iconbase[j];
        for (uint16_t i = 0; i < 8; i++)
        {
            if (temp & 0x80)
            {
                buffer[n] = fc;
            }
            else
            {
                buffer[n] = bc;
            }
            temp <<= 1;
            ++n;
            if (n == width)
            {
                n = 0;
                esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + width, y + 1, buffer);
                y++;
                break;
            }
        }
    }
    
    heap_caps_free(buffer);
}
