#include "touch.h"

_m_tp_dev tp_dev = {
    ctp_init,
    0,      /*剩下成员会自动初始化为0*/
};

uint8_t ctp_init(i2c_obj_t self)
{
    uint8_t ret = 1;
    tp_dev.touchtype = 0;       /*默认为电阻屏，竖屏*/

    if(ltdcdev.id == 0x4384 || ltdcdev.id == 0x4342)
    {
        while (1)
        {
            ret = gt9xxx_init(self);
            if (ret)
            {
                printf("gt9xxx_init fail!\n");
            }
            else
            {
                break;
            }
            vTaskDelay(100);
        }

        tp_dev.scan = gt9xxx_scan;
        tp_dev.touchtype |= ltdcdev.dir & 0x01;     /*设置屏幕方向*/
        tp_dev.touchtype |= 0x80;                   /*设置为电容屏*/
        return 0;
    }

    return 1;
}

void load_draw_dialog(void)
{
    ltdc_clear(WHITE);
    ltdc_show_string(ltdcdev.width - 48, 0, "RST", 32, BLUE, WHITE, 1);
}

void ctp_draw_bline(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint8_t size, uint16_t color)
{
    int delta_x, delta_y;
    uint16_t row, col;
    int incx, incy;
    uint16_t distance;
    uint16_t xerr = 0, yerr = 0;

    if (sx < size || sy < size || ex < size || ey < size)
    {
        return;
    }

    delta_x = ex - sx;
    delta_y = ey - sy;
    row = sx;
    col = sy;

    /*设置单步方向*/
    if (delta_x > 0)
    {
        incx = 1;
    }
    else if (delta_x == 0)
    {
        incx = 0;       /*竖线*/
    }
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)
    {
        incy = 1;
    }
    else if (delta_y == 0)
    {
        incy = 0;       /*横线*/
    }
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    /*选取基本坐标增量*/
    if (delta_x > delta_y)
    {
        distance = delta_x;
    }
    else
    {
        distance = delta_y;
    }

    /*画粗线输出*/
    for (uint16_t i = 0; i <= distance + 1; i++)
    {
        xerr += delta_x;
        yerr += delta_y;
        ltdc_draw_circle(row, col, size, color);
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

/*6个触点的颜色（电容触摸屏用）*/
static const uint16_t POINT_COLOR_TBL[6] = {
    RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN,
};

void ctp_test(void)
{
    uint16_t lastpot[10][2];
    uint8_t t = 0;

    while (1)
    {
        tp_dev.scan(0);

        uint16_t active_touch_points = 0;   /*新增变量记录活跃同时触点数量*/
        /*开始画线*/
        for (uint8_t i = 0; i < gt_tnum; i++)
        {
            if (tp_dev.sta & (1 << i))      /*发现触点*/
            {
                active_touch_points++;      /*发现触摸点，计数器加1*/
                /*坐标在屏幕范围内*/
                if (tp_dev.x[i] < ltdcdev.width && tp_dev.y[i] < ltdcdev.height)
                {
                    if (lastpot[i][0] == 0xFFFF)
                    {
                        lastpot[i][0] = tp_dev.x[i];
                        lastpot[i][1] = tp_dev.y[i];
                    }
                    ctp_draw_bline(lastpot[i][0], lastpot[i][1], tp_dev.x[i], tp_dev.y[i], 2, POINT_COLOR_TBL[i]);  /*画粗线*/
                    lastpot[i][0] = tp_dev.x[i];
                    lastpot[i][1] = tp_dev.y[i];
                    if (tp_dev.x[i] > ltdcdev.width - 48 && tp_dev.y[i] < 50)
                    {
                        load_draw_dialog(); /*清屏*/
                    }
                }
            }
            else
            {
                lastpot[i][0] = 0xFFFF;
            }
        }

        vTaskDelay(15);
        t++;

        if (t % 100 == 0)
        {
            t = 0;
            LED_TOGGLE();
            printf("Active touch point: %d\n", active_touch_points);
        }
    }
}

