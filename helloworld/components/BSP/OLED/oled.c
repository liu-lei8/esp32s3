#include "oled.h"
#include "oledfont.h"

i2c_obj_t oled_master;
uint8_t oled_gram[128][8] = {0};

void oled_init(i2c_obj_t self)
{
    if (self.init_flag == ESP_FAIL)
    {
        oled_master = iic_init(I2C_NUM_0);
    }
    else
    {
        oled_master = self;
    }

    /*配置复位引脚电平*/
    OLED_RST(1);
    vTaskDelay(100);
    /*复位OLED*/
    OLED_RST(0);
    vTaskDelay(200);
    OLED_RST(1);

    oled_initialize4();

    /*oled清屏*/
    oled_clear();
}

void oled_on(void)
{
    oled_write_Byte(0xAF, OLED_CMD);
}

void oled_off(void)
{
    oled_write_Byte(0xAE,OLED_CMD);
}

void oled_clear(void)
{
    // uint8_t oled_data = OLED_DATA;  /*写数据控制字节*/
    // uint8_t clear_buffer[128];      /*128个数据*/
    // memset(clear_buffer, 0x00, sizeof(clear_buffer));

    // i2c_buf_t bufs[2] = {
    //     {.buf = &oled_data, .len = 1},
    //     {.buf = clear_buffer, .len = 128}
    // };

    // for (uint8_t i = 0; i < 8; i++)
    // {
    //     oled_write_Byte(0xB0 + i, OLED_CMD);        /*设置页地址*/
    //     oled_write_Byte(0x00, OLED_CMD);            /*设置列低地址*/
    //     oled_write_Byte(0x10, OLED_CMD);            /*设置列高地址*/

    //     iic_transfer(&oled_master, OLED_ADDR, 2, bufs, IIC_FLAG_STOP);
    // }

    /*有些oled应该不支持上面的连续数据发送，只能一个个数据发送*/
    for (uint8_t i = 0; i < 8; i++)
    {
         for (uint8_t j = 0; j < 128; j++)
         {
            oled_gram[j][i] = 0;
         }
    }
    oled_reflash_gram();
}

void oled_reflash_gram(void)
{
    for (uint8_t i = 0; i < 8; i++)
    {
         oled_write_Byte(0xB0 + i, OLED_CMD);
         oled_write_Byte(0x00, OLED_CMD);
         oled_write_Byte(0x10, OLED_CMD);

         for (uint8_t j = 0; j < 128; j++)
         {
            oled_write_Byte(oled_gram[j][i], OLED_DATA);
         }
    }
}

void oled_write_Byte(unsigned char tx_data, unsigned char command)
{
    uint8_t buf[2] = {command, tx_data};
    oled_write(buf, sizeof(buf));
}

esp_err_t oled_write(uint8_t* data_wr, size_t size)
{
    i2c_buf_t bufs = {.buf = data_wr, .len = size};
    esp_err_t err = iic_transfer(&oled_master, OLED_ADDR, 1, &bufs, IIC_FLAG_STOP);

    return err;
}

void oled_draw_point(uint8_t x, uint8_t y, uint8_t dot)
{
    if (x > 127 || y > 63)
    {
        return;
    }

    uint8_t pos = y / 8;            /*得到页数*/
    uint8_t mask = 1 << (y % 8);    /*得到位掩码*/

    if (dot)
    {
        oled_gram[x][pos] |= mask;      /*打开位*/
    }
    else
    {
        oled_gram[x][pos] &= ~mask;     /*关闭位*/
    }
}

void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t mode)
{
    uint8_t temp;
    uint8_t* pfont = NULL;
    uint8_t y0 = y;

    uint8_t csize = ((size / 8) + ((size % 8) ? 1 : 0)) * (size / 2);   /*一个字符所占字节大小*/

    chr = chr - ' ';            /*减去ASCII码得到的偏置值*/
    if (size == 12)
    {
        pfont = (uint8_t*)oled_asc2_1206[chr];
    }
    else if (size == 16)
    {
        pfont = (uint8_t*)oled_asc2_1608[chr];
    }
    else if (size == 24)
    {
        pfont = (uint8_t*)oled_asc2_2412[chr];
    }

    for (uint8_t i = 0; i < size; i++)
    {
        temp = pfont[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (temp & 0x80)
            {
                oled_draw_point(x, y, mode);
            }
            else
            {
                oled_draw_point(x, y, !mode);
            }
            temp <<= 1;
            ++y;
            if (y - y0 == size)
            {
                y = y0;
                ++x;
                break;
            }
        }
    }
}

void oled_show_string(uint8_t x, uint8_t y, char* string, uint8_t size)
{
    while (*string != '\0')
    {
        oled_show_char(x, y, *string, size, 1);
        string++;
        x += size / 2;
    }
}

void oled_show_num(uint8_t x, uint8_t y, uint8_t chr, uint8_t size)
{
    uint8_t ascii = chr;
    uint8_t uint = 0;           /*个位*/
    uint8_t decade = 0;         /*十位*/
    uint8_t hundred = 0;        /*百位*/

    uint = ascii % 10;
    decade = ascii / 10;
    decade %= 10;
    hundred = ascii / 100;

    oled_show_char(x, y, hundred + 48, size, 1);        /*加48就是数字的ascii码*/
    x += size / 2;
    oled_show_char(x, y, decade + 48, size, 1);
    x += size / 2;
    oled_show_char(x, y, uint + 48, size, 1);
}

void oled_initialize4(void)
{
    /*初始化代码*/
    // oled_write_Byte(0xAE,OLED_CMD);/*关闭显示*/
    // oled_write_Byte(0xD5,OLED_CMD);/*设置时钟分频因子,震荡频率*/
    // oled_write_Byte(0xF0,OLED_CMD);/* [3:0],分频因子;[7:4],震荡频率*/
    // oled_write_Byte(0xA8,OLED_CMD);/*设置驱动路数*/
    // oled_write_Byte(0X3F,OLED_CMD);/*默认0X3F(1/64) */
    // oled_write_Byte(0xD3,OLED_CMD);/*设置显示偏移*/
    // oled_write_Byte(0X00,OLED_CMD);/*默认为0 */
    // oled_write_Byte(0x40,OLED_CMD);/*设置显示开始行[5:0],行数*/
    // /*段重定义设置,bit0:0,0->0;1,0->127;A1是正常显示，A0左右翻转 */
    // oled_write_Byte(0xA1,OLED_CMD);
    // /*设置COM扫描方向;bit3:0,普通模式;1,重定义模式COM[N-1]->COM0;N:驱动路数*/
    // oled_write_Byte(0xC8,OLED_CMD);/*C0会上下翻转，C8是正常显示*/
    // oled_write_Byte(0xDA,OLED_CMD);/*设置COM硬件引脚配置*/
    // oled_write_Byte(0x02,OLED_CMD);/* [5:4]配置*/
    // oled_write_Byte(0x81,OLED_CMD);/*对比度设置*/
    // oled_write_Byte(0x7F,OLED_CMD);/* 1~255;默认0X7F (亮度设置,越大越亮) */
    // oled_write_Byte(0xD9,OLED_CMD);/*设置预充电周期*/
    // oled_write_Byte(0xF1,OLED_CMD);/* [3:0],PHASE 1;[7:4],PHASE 2; */
    // oled_write_Byte(0xDB,OLED_CMD);/*设置VCOMH电压倍率*/
    // /*[6:4]000,0.65*vcc;001,0.77*vcc;011,0.83*vcc; */
    // oled_write_Byte(0x30,OLED_CMD);
    // oled_write_Byte(0x8D,OLED_CMD);/*电荷泵设置*/
    // oled_write_Byte(0x14,OLED_CMD);/* bit2，开启/关闭*/
    // oled_write_Byte(0x20,OLED_CMD);/*设置内存地址模式*/
    // /*[1:0],00，列地址模式;01，行地址模式;02,页地址模式;默认02; */
    // oled_write_Byte(0x02,OLED_CMD);
    // /*全局显示开启;bit0:1,开启;0,关闭;(白屏/黑屏) */
    // oled_write_Byte(0xA4,OLED_CMD);
    // oled_write_Byte(0xA6,OLED_CMD);/*设置显示方式;bit0:1,反相显示;0,正常显示*/

    // vTaskDelay(pdMS_TO_TICKS(100));         // 等待电荷泵稳定
    // oled_write_Byte(0xAF,OLED_CMD);/*开启显示*/

    // vTaskDelay(pdMS_TO_TICKS(200));/*在 AFh 之后增加至少 100ms 延时（手册要求 tAF = 100ms）*/



    /*4脚oled商家发给我的初始化序列，可以正常显示*/
    oled_write_Byte(0xAE,OLED_CMD);//--turn off oled panel
    oled_write_Byte(0x00,OLED_CMD);//---set low column address
    oled_write_Byte(0x10,OLED_CMD);//---set high column address
    oled_write_Byte(0x40,OLED_CMD);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
    oled_write_Byte(0x81,OLED_CMD);//--set contrast control register
    oled_write_Byte(0xCF,OLED_CMD);// Set SEG Output Current Brightness
    oled_write_Byte(0xA1,OLED_CMD);//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
    oled_write_Byte(0xC8,OLED_CMD);//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
    oled_write_Byte(0xA6,OLED_CMD);//--set normal display
    oled_write_Byte(0xA8,OLED_CMD);//--set multiplex ratio(1 to 64)
    oled_write_Byte(0x3f,OLED_CMD);//--1/64 duty
    oled_write_Byte(0xD3,OLED_CMD);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
    oled_write_Byte(0x00,OLED_CMD);//-not offset
    oled_write_Byte(0xd5,OLED_CMD);//--set display clock divide ratio/oscillator frequency
    oled_write_Byte(0x80,OLED_CMD);//--set divide ratio, Set Clock as 100 Frames/Sec
    oled_write_Byte(0xD9,OLED_CMD);//--set pre-charge period
    oled_write_Byte(0xF1,OLED_CMD);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
    oled_write_Byte(0xDA,OLED_CMD);//--set com pins hardware configuration
    oled_write_Byte(0x12,OLED_CMD);
    oled_write_Byte(0xDB,OLED_CMD);//--set vcomh
    oled_write_Byte(0x40,OLED_CMD);//Set VCOM Deselect Level
    oled_write_Byte(0x20,OLED_CMD);//-Set Page Addressing Mode (0x00/0x01/0x02)
    oled_write_Byte(0x02,OLED_CMD);//
    oled_write_Byte(0x8D,OLED_CMD);//--set Charge Pump enable/disable
    oled_write_Byte(0x14,OLED_CMD);//--set(0x10) disable
    oled_write_Byte(0xA4,OLED_CMD);// Disable Entire Display On (0xa4/0xa5)
    oled_write_Byte(0xA6,OLED_CMD);// Disable Inverse Display On (0xa6/a7) 
    oled_write_Byte(0xAF,OLED_CMD);

}

void oled_initialize7(void)
{
    /*下面两种初始化序列是7脚oled商家给的*/
    // oled_write_Byte(0xae, OLED_CMD);	//--turn off oled panel
    // oled_write_Byte(0x00, OLED_CMD);	//--set low column address
    // oled_write_Byte(0x10, OLED_CMD);	//--set high column address
    // oled_write_Byte(0x40, OLED_CMD);	//--set start line address
    // oled_write_Byte(0x81, OLED_CMD);	//--set contrast control register
    // oled_write_Byte(0xf0, OLED_CMD);

    // oled_write_Byte(0xa0, OLED_CMD);	//--set segment re-map a0--0 to 127    a1---127to 0

    // oled_write_Byte(0xa6, OLED_CMD);	//--set normal display

    // oled_write_Byte(0xc0, OLED_CMD);	//--set com(N-1)to com0  c0:com0 to com(N-1)

    // oled_write_Byte(0xa8, OLED_CMD);	//--set multiples ratio(1to64)
    // oled_write_Byte(0x3f, OLED_CMD);	//--1/64 duty
    // oled_write_Byte(0xd3, OLED_CMD);	//--set display offset
    // oled_write_Byte(0x00, OLED_CMD);	//--not offset
    // oled_write_Byte(0xd5, OLED_CMD);	//--set display clock divide ratio/oscillator frequency
    // oled_write_Byte(0x80, OLED_CMD);	//--set divide ratio
    // oled_write_Byte(0xd9, OLED_CMD);	//--set pre-charge period
    // oled_write_Byte(0xf1, OLED_CMD);
    // oled_write_Byte(0xda, OLED_CMD);	//--set com pins hardware configuration
    // oled_write_Byte(0x12, OLED_CMD);
    // oled_write_Byte(0xdb, OLED_CMD);	//--set vcomh
    // oled_write_Byte(0x30, OLED_CMD);
    // oled_write_Byte(0x8d, OLED_CMD);	//--set chare pump enable/disable
    // oled_write_Byte(0x14, OLED_CMD);	//--set(0x10) disable
    // oled_write_Byte(0xaf, OLED_CMD);	//--turn on oled panel


    // oled_write_Byte(0xae, OLED_CMD);	//--turn off oled panel
    // oled_write_Byte(0x00, OLED_CMD);	//--set low column address
    // oled_write_Byte(0x10, OLED_CMD);	//--set high column address
    // oled_write_Byte(0x40, OLED_CMD);	//--set start line address
    // oled_write_Byte(0x81, OLED_CMD);	//--set contrast control register
    // oled_write_Byte(0x8f, OLED_CMD);
    // oled_write_Byte(0xa1, OLED_CMD);	//--set segment re-map 95 to 0
    // oled_write_Byte(0xa6, OLED_CMD);	//--set normal display
    // oled_write_Byte(0xa8, OLED_CMD);	//--set multiples ratio(1to64)
    // oled_write_Byte(0x3f, OLED_CMD);	//--1/64 duty
    // oled_write_Byte(0xd3, OLED_CMD);	//--set display offset
    // oled_write_Byte(0x00, OLED_CMD);	//--not offset
    // oled_write_Byte(0xd5, OLED_CMD);	//--set display clock divide ratio/oscillator frequency
    // oled_write_Byte(0x80, OLED_CMD);	//--set divide ratio
    // oled_write_Byte(0xd9, OLED_CMD);	//--set pre-charge period
    // oled_write_Byte(0x22, OLED_CMD);
    // oled_write_Byte(0xda, OLED_CMD);	//--set com pins hardware configuration
    // oled_write_Byte(0x12, OLED_CMD);
    // oled_write_Byte(0xdb, OLED_CMD);	//--set vcomh
    // oled_write_Byte(0x30, OLED_CMD);
    // oled_write_Byte(0x8d, OLED_CMD);	//--set chare pump enable/disable
    // oled_write_Byte(0x10, OLED_CMD);	//--set(0x14) enable
    // oled_write_Byte(0xaf, OLED_CMD);	//--turn on oled panel


    /*这个是商家文档里的SSD1306-SPI驱动的初始化序列*/
    oled_write_Byte(0xae, OLED_CMD);

    oled_write_Byte(0x40, OLED_CMD);

    oled_write_Byte(0x20, OLED_CMD);
    oled_write_Byte(0x02, OLED_CMD);
    
    oled_write_Byte(0x81, OLED_CMD);
    oled_write_Byte(0xff, OLED_CMD);

    oled_write_Byte(0xa1, OLED_CMD);
    oled_write_Byte(0xa4, OLED_CMD);
    oled_write_Byte(0xa6, OLED_CMD);
    oled_write_Byte(0xc8, OLED_CMD);

    oled_write_Byte(0xa8, OLED_CMD);
    oled_write_Byte(0x3f, OLED_CMD);

    oled_write_Byte(0xd5, OLED_CMD);
    oled_write_Byte(0x80, OLED_CMD);

    oled_write_Byte(0xd3, OLED_CMD);
    oled_write_Byte(0x00, OLED_CMD);

    oled_write_Byte(0x8d, OLED_CMD);
    oled_write_Byte(0x14, OLED_CMD);

    oled_write_Byte(0xdb, OLED_CMD);
    oled_write_Byte(0x40, OLED_CMD);

    oled_write_Byte(0xd9, OLED_CMD);
    oled_write_Byte(0xf1, OLED_CMD);

    oled_write_Byte(0xaf, OLED_CMD);
}
