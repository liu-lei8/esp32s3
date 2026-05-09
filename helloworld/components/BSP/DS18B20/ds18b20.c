#include "ds18b20.h"

const uint8_t ds18b20_rom0[8] = {0x28, 0x0F, 0x31, 0x87, 0x00, 0xE2, 0x3B, 0x56};
const uint8_t ds18b20_rom1[8] = {0x28, 0x95, 0x32, 0x87, 0x00, 0x3B, 0x3F, 0xED};


static void ds18b20_write_byte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x01)
        {
            DS18B20_DQ_OUT(0);
            esp_rom_delay_us(2);
            DS18B20_DQ_OUT(1);
            esp_rom_delay_us(60);
        }
        else
        {
            DS18B20_DQ_OUT(0);
            esp_rom_delay_us(60);
            DS18B20_DQ_OUT(1);
            esp_rom_delay_us(2);
        }
        data >>= 1;
    }
}

static void ds18b20_set_resolution(uint8_t resolution)
{
    /*分辨率由暂存器 第 5 字节（配置寄存器） 的 第 5 位 (R0) 和 第 6 位 (R1) 控制*/
    ds18b20_reset();
    ds18b20_check();
    ds18b20_write_byte(0xCC);           /*广播指令，对所有设备寻址*/
    ds18b20_write_byte(0x4E);           /*写暂存器命令*/
    ds18b20_write_byte(0xFF);           /*TH寄存器，高温报警阈值（字节1，随意值）*/
    ds18b20_write_byte(0x00);           /*TL寄存器，低温报警阈值（字节2，随意值）*/
    ds18b20_write_byte(resolution);     /*配置寄存器（字节3，设置分辨率）*/
}

uint8_t ds18b20_init(void)
{
    gpio_config_t ds18b20_gpio_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,  /*开漏输入输出模式*/
        .pin_bit_mask = 1ull << DS18B20_DQ_PIN,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&ds18b20_gpio_cfg);

    ds18b20_set_resolution(DS18B20_RES_12_BIT); /*可以不设置，默认也是12位分辨率*/
    ds18b20_reset();
    return ds18b20_check();
}

void ds18b20_reset(void)
{
    DS18B20_DQ_OUT(0);      /*拉低DQ，复位*/
    esp_rom_delay_us(720);
    DS18B20_DQ_OUT(1);      /*释放DQ*/
    esp_rom_delay_us(15);
}

uint8_t ds18b20_check(void)
{
    uint8_t flag = 0;
    uint8_t retry = 0;

    while (DS18B20_DQ_IN && retry < 200)        /*等待200us，直到DQ引脚被DS18B20拉低*/
    {
        ++retry;
        esp_rom_delay_us(1);
    }

    if (retry >= 200)
    {
        flag = 1;
    }
    else
    {
        retry = 0;

        while (!DS18B20_DQ_IN && retry < 240)   /*等待240us，直到DQ引脚被拉高*/
        {
            ++retry;
            esp_rom_delay_us(1);
        }

        if(retry >= 240)
        {
            flag = 1;
        }
    }

    return flag;
}

static uint8_t ds18b20_read_bit(void)
{
    uint8_t data = 0;

    DS18B20_DQ_OUT(0);
    esp_rom_delay_us(2);
    DS18B20_DQ_OUT(1);
    esp_rom_delay_us(12);

    if (DS18B20_DQ_IN)
    {
        data = 1;
    }
    esp_rom_delay_us(50);

    return data;
}

static uint8_t ds18b20_read_byte(void)
{
    uint8_t data = 0;
    uint8_t bit = 0;

    for (uint8_t i = 0; i < 8; i++)
    {
        bit = ds18b20_read_bit();
        data |= bit << i;
    }
    
    return data;
}

short ds18b20_get_temperature(void)
{
    uint8_t flag = 1;
    uint8_t TL, TH;
    short temp;

    ds18b20_start_convert();

    ds18b20_reset();
    ds18b20_check();
    ds18b20_write_byte(0xCC);
    ds18b20_write_byte(0xBE);       /*获取暂存器温度数据*/
    TL = ds18b20_read_byte();
    TH = ds18b20_read_byte();

    if (TH > 7) /*高八位，第四位开始后面的位都表示正负，所以大于7表示温度为负值*/
    {   /*温度AD值的表示法和计算机存储正负数据的原理一致：用补码进行计算，此时只取反并没有加一，此时加一可能会消除进位，所以等后面两个数据合并时再加一*/
        TL = ~TL;
        TH = ~TH;
        flag = 0;                   /*温度为负*/
    }

    temp = TH;      /*获得高八位*/
    temp <<= 8;
    temp |= TL;     /*获得低八位*/

    if (flag == 0)
    {
        temp = (temp + 1) * 0.625;
        temp = -temp;
    }
    else
    {
        temp = temp * 0.625;
    }

    return temp;
}

void ds18b20_read_rom(void)
{
    uint8_t rom_array[8];

    ds18b20_reset();
    assert(!ds18b20_check());

    ds18b20_write_byte(0x33);   /*发送读rom命令*/
    for (uint8_t i = 0; i < 8; i++)
    {
        rom_array[i] = ds18b20_read_byte();
    }
    printf("DS18B20 ROM序列号：");
    for (uint8_t i = 0; i < 8; i++)
    {
        printf("%02X ", rom_array[i]);
    }
    printf("\r\n");
}

static void ds18b20_match_rom(const uint8_t* rom)
{
    ds18b20_reset();
    ESP_ERROR_CHECK(ds18b20_check());

    ds18b20_write_byte(0x55);       /*发送匹配rom指令*/
    for (uint8_t i = 0; i < 8; i++)
    {
        ds18b20_write_byte(rom[i]); /*发送8字节的序列号进行比对*/
    }
}

short ds18b20_get_temperature_rom(const uint8_t* rom)
{
    uint8_t TL, TH;
    short temp = 0;

    /*第一步：指定设备启动模数转换(这里也可以用0xCC广播启动转换,然后逐一获取温度的方式)*/
    // ds18b20_match_rom(rom);
    // ds18b20_write_byte(0x44);   /*开始转换*/
    // vTaskDelay(750);

    /*第二步：获取指定设备的温度数据*/
    ds18b20_match_rom(rom);
    ds18b20_write_byte(0xBE);   /*从暂存器开始获取数据*/
    TL = ds18b20_read_byte();
    TH = ds18b20_read_byte();

    temp = (TH << 8) | TL;
    if (TH > 7)    /*说明温度为负*/
    {
        temp = (~temp + 1) * 0.625 * (-1);
    }
    else
    {
        temp = (double)temp * 256 / 4096 * 10;  /*转换为double类型数据，除完后小数部分不会被截断*/
    }

    return temp;
}

void ds18b20_start_convert(void)
{
    ds18b20_reset();
    ds18b20_check();
    ds18b20_write_byte(0xCC);   /*广播指令，对所有设备寻址*/
    ds18b20_write_byte(0x44);   /*开始模数转换*/
    vTaskDelay(750);
}
