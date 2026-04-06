#include "lcd.h"

lcd_obj_t lcd_self;
spi_device_handle_t my_lcd_handle;

void lcd_init(void)
{
    int cmd = 0;
    esp_err_t ret;

    lcd_self.dir = 0;
    lcd_self.cs = LCD_NUM_CS;
    lcd_self.wr = LCD_NUM_WR;

    gpio_config_t wr_gpio_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ull << lcd_self.wr,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&wr_gpio_cfg);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_MASTER_FREQ_10M,
        .mode = 3,
        .spics_io_num = -1,     /*片选线直接接地了*/
        .queue_size = 7,
    };
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &my_lcd_handle);
    ESP_ERROR_CHECK(ret);

    LCD_RST(1);
    LCD_WR(1);
    LCD_PWR(1);
    vTaskDelay(pdMS_TO_TICKS(100));

    lcd_hard_reset();
    lcd_initialize();


    lcd_clear(WHITE);
}

void lcd_write_cmd(const uint8_t cmd)
{
    LCD_WR(0);
    spi2_write_cmd(my_lcd_handle, cmd);
}

void lcd_write_data(uint16_t* data, int len)
{
    if (!gpio_get_level(LCD_NUM_WR))
    {
        LCD_WR(1);
    }
    uint8_t* temp = (uint8_t*)data;
    for (uint16_t i = 0; i < len; i++)
    {
        data[i] = (data[i] >> 8) | (data[i] << 8);
    }
    
    spi2_write_data(my_lcd_handle, temp, len * 2);
}

void lcd_write_data8(uint8_t data)
{
    LCD_WR(1);
    spi2_write_cmd(my_lcd_handle, data);
}

void lcd_write_data16(uint16_t data)
{
    uint8_t buf[2] = {0};
    buf[0] = data >> 8;     /*已经对颜色值的高八位和第八位进行翻转了*/
    buf[1] = data & 0xFF;
    LCD_WR(1);
    spi2_write_data(my_lcd_handle, buf, 2);
}

void lcd_hard_reset(void)
{
    LCD_RST(0);
    vTaskDelay(pdMS_TO_TICKS(100));
    LCD_RST(1);
    vTaskDelay(pdMS_TO_TICKS(150));
}

void lcd_initialize(void)
{
    /*厂家（浦洋P169H002,1.69寸lcd）初始化序列*/
    lcd_write_cmd(0x11);                /*sleep out*/
    vTaskDelay(pdMS_TO_TICKS(120));

    // lcd_write_cmd(0x01);                /*软件复位，可选项*/
    // vTaskDelay(pdMS_TO_TICKS(150));

    lcd_write_cmd(0x36);                /*设置横竖屏*/
    if (USE_HORIZONTAL == 0)
    {
        lcd_write_data8(0x00);          /*尝试0x00为RGB模式,0X08为BGR模式*/
    }
    else if (USE_HORIZONTAL == 1)
    {
        lcd_write_data8(0xC0);
    }
    else if (USE_HORIZONTAL == 2)
    {
        lcd_write_data8(0x70);
    }
    else
    {
        lcd_write_data8(0xA0);
    }

    lcd_write_cmd(0x3A);        /*颜色格式*/
    lcd_write_data8(0x55);      /*16位RGB565模式*,官方提供的0x05是RGB444格式*/

    // lcd_write_cmd(0xB2);
    // lcd_write_data8(0x0C);
    // lcd_write_data8(0x0C);
    // lcd_write_data8(0x00);
    // lcd_write_data8(0x33);
    // lcd_write_data8(0x33);

    // lcd_write_cmd(0xB7);
    // lcd_write_data8(0x35);

    // lcd_write_cmd(0xBB);
    // lcd_write_data8(0x32);      /*Vcom = 1.35V*/

    // lcd_write_cmd(0xC2);
    // lcd_write_data8(0x01);

    // lcd_write_cmd(0xC3);
    // lcd_write_data8(0x15);      /*GVDD = 4.8V  颜色深度*/

    // lcd_write_cmd(0xC4);
    // lcd_write_data8(0x20);      /*VDV, 0x20:0v*/

    // lcd_write_cmd(0xC6);
    // lcd_write_data8(0x0F);      /*0x0F:60Hz*/

    // lcd_write_cmd(0xD0);
    // lcd_write_data8(0xA4);
    // lcd_write_data8(0xA1);

    // lcd_write_cmd(0xE0);
    // lcd_write_data8(0xD0);
    // lcd_write_data8(0x08);
    // lcd_write_data8(0x0E);
    // lcd_write_data8(0x09);
    // lcd_write_data8(0x09);
    // lcd_write_data8(0x05);
    // lcd_write_data8(0x31);
    // lcd_write_data8(0x33);
    // lcd_write_data8(0x48);
    // lcd_write_data8(0x17);
    // lcd_write_data8(0x14);
    // lcd_write_data8(0x15);
    // lcd_write_data8(0x31);
    // lcd_write_data8(0x34);

    // lcd_write_cmd(0xE1);
    // lcd_write_data8(0xD0);
    // lcd_write_data8(0x08);
    // lcd_write_data8(0x0E);
    // lcd_write_data8(0x09);
    // lcd_write_data8(0x09);
    // lcd_write_data8(0x15);
    // lcd_write_data8(0x31);
    // lcd_write_data8(0x33);
    // lcd_write_data8(0x48);
    // lcd_write_data8(0x17);
    // lcd_write_data8(0x14);
    // lcd_write_data8(0x15);
    // lcd_write_data8(0x31);
    // lcd_write_data8(0x34);
    lcd_write_cmd(0x21);         /*0x20开启反色，0x21为关闭反色*/

    lcd_write_cmd(0x29);            /*开启显示*/
    vTaskDelay(pdMS_TO_TICKS(20));
}

void lcd_address_set(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2)
{
    if (USE_HORIZONTAL == 0)
    {
        lcd_write_cmd(0x2a);        /*列地址设置*/
        lcd_write_data16(x1);
        lcd_write_data16(x2);
        lcd_write_cmd(0x2b);        /*行地址设置*/
        lcd_write_data16(y1 + 20);
        lcd_write_data16(y2 + 20);
        lcd_write_cmd(0x2c);        /*写到gram里*/
    }
    else if (USE_HORIZONTAL == 1)
    {
        lcd_write_cmd(0x2a);        /*列地址设置*/
        lcd_write_data16(x1);
        lcd_write_data16(x2);
        lcd_write_cmd(0x2b);        /*行地址设置*/
        lcd_write_data16(y1 + 80);
        lcd_write_data16(y2 + 80);
        lcd_write_cmd(0x2c);        /*存储器写*/
    }
    else if (USE_HORIZONTAL == 2)
    {
        lcd_write_cmd(0x2a);        /*列地址设置*/
        lcd_write_data16(x1 + 20);
        lcd_write_data16(x2 + 20);
        lcd_write_cmd(0x2b);        /*行地址设置*/
        lcd_write_data16(y1);
        lcd_write_data16(y2);
        lcd_write_cmd(0x2c);        /*存储器写*/
    }
    else
    {
        lcd_write_cmd(0x2a);        /*列地址设置*/
        lcd_write_data16(x1 + 80);
        lcd_write_data16(x2 + 80);
        lcd_write_cmd(0x2b);        /*行地址设置*/
        lcd_write_data16(y1);
        lcd_write_data16(y2);
        lcd_write_cmd(0x2c);        /*存储器写*/
    }
}

void lcd_fill(uint16_t xstart, uint16_t xend, uint16_t ystart, uint16_t yend, uint16_t color)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    uint32_t pixelCount = (xend - xstart + 1) * (yend - ystart + 1);
    /*ESP32 是小端模式：在内存中存储一个16位数据时，低字节存放在低地址，高字节存放在高地址。当通过 SPI 硬件发送 16 位数据时，SPI 外设会先发送低字节，后发送高字节。
    很多 LCD 控制器（尤其是使用 16 位 RGB565 格式的）期望大端顺序：即先收到颜色的高字节，再收到低字节。*/
    uint16_t color_buf = (uint16_t)((color >> 8) | (color << 8));

    lcd_address_set(xstart, xend, ystart, yend);
    LCD_WR(1);

    static uint16_t buffer[128];
    for (uint32_t i = 0; i < 128; i++)
    {
        buffer[i] = color_buf;
    }

    t.tx_buffer = buffer;

    uint32_t remaining = pixelCount;
    while (remaining > 0)
    {
        uint32_t chunk_size = (remaining > 128) ? 128 : remaining;
        t.length = chunk_size * sizeof(uint16_t) * 8;
        ret = spi_device_polling_transmit(my_lcd_handle, &t);
        ESP_ERROR_CHECK(ret);
        remaining -= chunk_size;
    }

}

void lcd_clear(uint16_t color)
{
    if (USE_HORIZONTAL == 0 || USE_HORIZONTAL == 1)
    {
        lcd_fill(0, 239, 0, 279, color);    /*竖屏*/
    }
    else
    {
        lcd_fill(0, 279, 0, 239, color);
    }
}

void lcd_draw_poiont(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_address_set(x, x, y, y);
    lcd_write_data16(color);
}

void lcd_show_char(uint16_t x, uint16_t y, uint8_t chr, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t sizex;
    uint8_t size;       /*若一个像素为一位，那么一个字符的字节大小为size*/
    const uint8_t* temp = NULL;         /*不要改变字符集的值*/
    uint8_t temp0;
    uint8_t x1  = 0, y1 = 0;
    uint8_t y0 = y1;
    uint16_t pixelCount;

    sizex = sizey / 2;
    uint16_t buf[sizex][sizey];
    size = ((sizey / 8) + ((sizey % 8) ? 1 : 0)) * sizex;
    pixelCount = sizex * sizey;

    chr = chr - ' ';
    if (sizey == 12)
    {
        temp = lcd_asc2_1206[chr];
    }
    else if (sizey == 16)
    {
        temp = lcd_asc2_1608[chr];
    }
    else if (sizey == 24)
    {
        temp = lcd_asc2_2412[chr];
    }
    else if (sizey == 32)
    {
        temp = lcd_asc2_3216[chr];
    }

    /*将所有颜色值从上到下从左到右的方式都写进缓冲区buf*/
    for (uint8_t i = 0; i < size; i++)
    {
        temp0 = temp[i];
        for (uint8_t byte = 0; byte < 8; byte++)
        {
            if (temp0 & 0x80)
            {
                buf[x1][y1] = fc;
            }
            else
            {
                buf[x1][y1] = bc;
            }
            ++y1;
            temp0 <<= 1;
            if (y1 - y0 == sizey)
            {
                ++x1;
                y1 = y0;
                break;
            }
        }
    }

    if (!mode)      /*非叠加模式，字体和新背景色会覆盖原先的背景色*/
    {
        lcd_address_set(x, x + sizex - 1, y, y + sizey - 1);
        for (uint8_t i = 0; i < sizey; i++)
        {
            for (uint8_t j = 0; j < sizex; j++)
            {
                lcd_write_data16(buf[j][i]);
            }
        }
    }
    else        /*叠加模式，只会写字体不会覆盖原先背景色*/
    {
        for (uint8_t i = 0; i < sizey; i++)
        {
            for (uint8_t j = 0; j < sizex; j++)
            {
                if (buf[j][i] == fc)
                {
                    lcd_draw_poiont(x + j, y + i, buf[j][i]);
                }
            }
        }
    }
}

void lcd_show_string(uint16_t x, uint16_t y, const char* string, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    while(*string != '\0')
    {
        lcd_show_char(x, y, *string, fc, bc, sizey, mode);
        x += sizey / 2;
        string++;
    }
}

uint8_t lcd_read_id(void)
{
    uint8_t id = 0;
    lcd_write_cmd(0x04);  // 正确的读ID命令
    
    // 需要多次传输获取完整ID
    id = spi2_transfer_byte(my_lcd_handle, 0xFF); // Dummy read
    id = spi2_transfer_byte(my_lcd_handle, 0xFF); // ID1
    id = spi2_transfer_byte(my_lcd_handle, 0xFF); // ID2
    id = spi2_transfer_byte(my_lcd_handle, 0xFF); // ID3
    
    return id;
}

void lcd_show_chinese(uint16_t x, uint16_t y, const char* chinese, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint16_t buf[sizey][sizey];
    uint8_t x1 = 0, y1 = 0;
    uint8_t y0 = y1;
    uint8_t HZ_NUM;             /*统计字符集中汉字数量，最多256个汉字*/
    const uint8_t* temp = NULL;
    uint8_t temp0;
    uint8_t size;               /*一个汉字像素所占字节数*/

    HZ_NUM = sizeof(tfont32) / sizeof(typFNT_GB32);
    size = sizey * sizey / 8;

    /*先根据索引查询汉字*/
    for (uint8_t n = 0; n < HZ_NUM; n++)
    {
        if (tfont32[n].index[0] == *chinese && tfont32[n].index[1] == *(chinese + 1) && tfont32[n].index[2] == *(chinese + 2))
        {
            /*省略其他汉字大小，目前只用32x32的*/
            if (sizey == 12)
            {
                //temp = tfont12[n].mask;
            }
            else if (sizey == 32)
            {
                temp = tfont32[n].mask;
            }

            /*先把汉字像素填充到缓冲区buf里,从上到下从左到右填充*/
            for (uint16_t i = 0; i < size; i++)
            {   temp0 = temp[i];
                for (uint8_t j = 0; j < 8; j++)
                {
                    if (temp0 & 0x80)
                    {
                        buf[x1][y1] = fc;
                    }
                    else
                    {
                        buf[x1][y1] = bc;
                    }
                    temp0 <<= 1;
                    ++y1;
                    if (y1 - y0 == sizey)
                    {
                        y1 = y0;
                        ++x1;
                        break;
                    }
                }
            }
            break;
        }
    }

    if (!mode)           /*非叠加模式*/
    {
        lcd_address_set(x, x + sizey - 1, y, y + sizey - 1);
        for (size_t i = 0; i < sizey; i++)
        {
            for (size_t j = 0; j < sizey; j++)
            {
                if (buf[j][i] == fc)
                {
                    lcd_write_data16(fc);
                }
                else
                {
                    lcd_write_data16(bc);
                }
            }
        }
    }
    else                /*叠加模式*/
    {
        for (size_t i = 0; i < sizey; i++)
        {
            for (size_t j = 0; j < sizey; j++)
            {
                if (buf[j][i] == fc)
                {
                    lcd_draw_poiont(x + j, y + i, fc);
                }
            }
        }
    }

}

void lcd_show_chinese_string(uint16_t x, uint16_t y, const char* chinese, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    while(*chinese != 0)
    {
        lcd_show_chinese(x, y, chinese, fc, bc, sizey, mode);
        x += sizey;
        chinese += 3;
    }
}

void lcd_show_picture(uint8_t picture, uint16_t fc, uint16_t bc, uint8_t mode)
{
    uint16_t x1 = 0, y1 = 0;
    uint16_t y0 = y1;
    uint16_t size = LCD_W * LCD_H / 8;
    const uint8_t* temp = NULL;
    uint8_t temp0 = 0;
    uint16_t line_buf[LCD_W];
    /*分配psram里的内存*/
    uint16_t (*buf)[LCD_H] = (uint16_t (*)[LCD_H])heap_caps_malloc(LCD_W * LCD_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (buf == NULL)
    {
        ESP_LOGE("LCD", "Failed to malloc memory for picture buffer");
        return;
    }

    if(picture == 0)
    {
        temp = picture0;
    }

    /*图像像素填充到buf*/
    for (uint16_t i = 0; i < size; i++)
    {
        temp0 = temp[i];
        for (uint16_t j = 0; j < 8; j++)
        {
            if (temp0 & 0x80)
            {
                buf[x1][y1] = fc;
            }
            else
            {
                buf[x1][y1] = bc;
            }
            ++y1;
            temp0 <<= 1;
            if (y1 - y0 == LCD_H)
            {
                y1 = y0;
                ++x1;
                break;
            }
        }
    }
    
    if (!mode)
    {
        lcd_address_set(0, LCD_W - 1, 0, LCD_H - 1);
        for (uint16_t i = 0; i < LCD_H; i++)
        {
            for (uint16_t j = 0; j < LCD_W; j++)
            {
                line_buf[j] = buf[j][i];
                // lcd_write_data16(buf[j][i]);
            }
            lcd_write_data(line_buf, LCD_W);
        }
    }
    else
    {
        for (uint16_t i = 0; i < LCD_H; i++)
        {
            for (uint16_t j = 0; j < LCD_W; j++)
            {
                if (buf[j][i] == fc)
                {
                    lcd_draw_poiont(j, i, fc);
                }
            }
        }
    }

    heap_caps_free(buf);
}

void lcd_show_picture0(uint8_t picture, uint16_t fc, uint16_t bc)
{
    const uint8_t* temp = picture0;
    if (temp == NULL) return;

    lcd_address_set(0, LCD_W - 1, 0, LCD_H - 1);
    LCD_WR(1);  // 准备写数据

    // 分配一行像素的颜色缓冲区（一维数组，每像素2字节）
    uint16_t *line_buf = (uint16_t*)heap_caps_malloc(LCD_W * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (line_buf == NULL) {
        ESP_LOGE("LCD", "malloc line buffer failed");
        return;
    }

    // 逐行处理：对于每一行 y
    for (uint16_t y = 0; y < LCD_H; y++) {
        // 对于当前行 y，提取该行所有 x 对应的像素颜色
        for (uint16_t x = 0; x < LCD_W; x++) {
            // 因为取模是列优先，位索引 = x * LCD_H + y
            uint32_t bit_idx = x * LCD_H + y;
            uint32_t byte_idx = bit_idx / 8;
            uint8_t bit = 7 - (bit_idx % 8);   // 高位在前
            uint8_t pixel = (temp[byte_idx] >> bit) & 0x01;
            line_buf[x] = pixel ? fc : bc;
        }
        // 将 line_buf 中的颜色值转换为 LCD 需要的字节序（高字节在前），并批量发送这一行
        uint8_t *byte_buf = (uint8_t*)line_buf;
        for (uint16_t x = 0; x < LCD_W; x++) {
            uint16_t swapped = (line_buf[x] >> 8) | (line_buf[x] << 8);
            line_buf[x] = swapped;
        }
        // 发送当前行的所有颜色数据（LCD_W * 2 字节）
        spi_transaction_t trans = {
            .length = LCD_W * 2 * 8,   // 位长度
            .tx_buffer = byte_buf,
        };
        spi_device_polling_transmit(my_lcd_handle, &trans);
    }

    heap_caps_free(line_buf);
}
