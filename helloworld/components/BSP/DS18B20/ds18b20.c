#include "ds18b20.h"

const uint8_t ds18b20_rom0[8] = {0x28, 0x0F, 0x31, 0x87, 0x00, 0xE2, 0x3B, 0x56};
const uint8_t ds18b20_rom1[8] = {0x28, 0x95, 0x32, 0x87, 0x00, 0x3B, 0x3F, 0xED};

ds18b20_sensor_t sensor_A = {
    .rom = ds18b20_rom0,
    .low_threshold = 100,
    .high_threshold = 200,
};

ds18b20_sensor_t sensor_B = {
    .rom = ds18b20_rom1,
    .low_threshold = 0,
    .high_threshold = 300,
};

/*指针数组方便循环处理*/
ds18b20_sensor_t* sensors[] = {&sensor_A, &sensor_B};

static void ds18b20_write_bit(uint8_t bit)
{
    if (bit)
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
}

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

void ds18b20_set_alarm(const uint8_t* rom, int8_t low_alarm, int8_t high_alarm)
{
    ds18b20_reset();
    ESP_ERROR_CHECK(ds18b20_check());
    ds18b20_match_rom(rom);
    ds18b20_write_byte(0x4E);       /*写暂存器命令*/
    ds18b20_write_byte((uint8_t)high_alarm);    /*TH寄存器（1℃精度，带符号）*/
    ds18b20_write_byte((uint8_t)low_alarm);     /*TL寄存器*/
    ds18b20_write_byte(DS18B20_RES_12_BIT);

    /**可选：将暂存器复制到EEPROM,掉电不丢失（命令0x48）
     * ds18b20_write_byte(0x48)
     * 复制到EEPROM需要10ms，这里略过
     */
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

/**
 * @brief 单总线ROM搜索（支持Search ROM和Alarm Search）
 * @param cmd   :0xF0(Search ROM)或0xEC(Alarm Search)
 * @param rom   :输出，8字节rom码
 * @param last_discr    :搜索状态变量（外部保存）。输出最晚一个存在差异的位，供寻找下个rom码回溯
 * @param last_family_discr :同上。输出8位家族码中最晚一个存在差异的位
 * @param last_device_flag  :同上。输出找到最后一个设备的标志。（0：还有设备，1：最后一个设备）
 * @return 1:找到设备，0：无设备
 * @see 二叉树搜索原理https://chat.deepseek.com/share/1xuv4n3hjv74glr0qu
 */
static int ow_search(uint8_t cmd, uint8_t* rom, uint8_t* last_discr, uint8_t* last_family_discr, uint8_t* last_device_flag)
{
    uint8_t id_bit_number;
    uint8_t last_zero, rom_byte_number, search_result;
    uint8_t id_bit, cmp_id_bit;
    uint8_t rom_byte_mask, search_dirction;

    /*初始化搜索变量*/
    id_bit_number = 1;
    last_zero = 0;
    rom_byte_number = 0;
    search_result = 0;
    rom_byte_mask = 1;

    /*如果上次搜索的不是最后一个设备,就进入判断继续搜索*/
    if (!(*last_device_flag))
    {
        ds18b20_reset();
        if (ds18b20_check() != 0)   /*如果没有设备应答，就重新开始*/
        {
            *last_discr = 0;
            *last_family_discr = 0;
            *last_device_flag = 0;
            return 0;
        }
        ds18b20_write_byte(cmd);    /*发送搜索命令*/

        /*循环搜索64位序列号*/
        do
        {
            /*读取一个位及其补码*/
            id_bit = ds18b20_read_bit();
            cmp_id_bit = ds18b20_read_bit();

            if (id_bit && cmp_id_bit) /*无设备相应*/
            {
                break;
            }

            if (id_bit != cmp_id_bit) /*所有设备该位相同，直接写入*/
            {
                search_dirction = id_bit;
            }
            else    /*该位存在冲突*/
            {
                if (id_bit_number < *last_discr) /*本次冲突位在上次冲突位之前，就走之前老路*/
                {
                    search_dirction = rom[rom_byte_number] & rom_byte_mask;
                }
                else    /*直到本次冲突位遇到上次冲突位，那么此次走另一条路1*/
                {       /*如果本次冲突位在上次冲突位之后，默认走0*/
                    search_dirction = (id_bit_number == *last_discr);
                }
                
                if(search_dirction == 0)
                {
                    last_zero = id_bit_number;  /*记录64位中上次冲突位走0的第几位的位数*/
                    /*检查是否为家族码的最后差异*/
                    if(last_zero < 9) *last_family_discr = last_zero;
                }
            }

            /*根据选择的方向设置rom字节位*/
            if (search_dirction)
            {
                rom[rom_byte_number] |= rom_byte_mask;
            }
            else
            {
                rom[rom_byte_number] &= ~rom_byte_mask;
            }

            /*发送方向位，不是该方向的设备退出本次搜索*/
            ds18b20_write_bit(search_dirction);
            id_bit_number++;
            rom_byte_mask <<= 1;

            if (rom_byte_mask == 0)
            {
                rom_byte_number++;
                rom_byte_mask = 1;
            }
        } while (rom_byte_number < 8);

        if (id_bit_number >= 65)    /*找到设备*/
        {
            *last_discr = last_zero;    /*记录最后一次选0的冲突位，供搜寻下个设备来回溯*/
            if (*last_discr == 0) /*说明没有冲突位了，这也就是最后一个设备了*/
            {
                *last_device_flag = 1;  /*0：不是最后一个设备，1：最后一个设备*/
            }
            search_result = 1;
        }
    }

    if (!search_result || !rom[0])  /*未找到设备，则重置搜索状态*/
    {
        *last_discr = 0;
        *last_family_discr = 0;
        *last_device_flag = 0;
        search_result = 0;
    }

    return search_result;
}

int ds18b20_alarm_search(void)
{
    uint8_t rom[8] = { 0 };
    int found = 0;

    /*初始化搜索状态*/
    uint8_t last_discrepancy = 0;
    uint8_t last_family_discrepancy = 0;
    uint8_t last_device_flag = 0;
    int search_result = 0;

    do 
    {   
        /*执行一次报警搜索，得到一个器件的rom*/
        search_result = ow_search(0xEC, &rom[0], &last_discrepancy, &last_family_discrepancy, &last_device_flag);   /*先搜索到sensor_B，再搜索到sensor_A*/
        if (search_result)
        {
            for (uint8_t i = 0; i < DS18B20_COUNT; i++)
            {
                if (memcmp(rom, sensors[i]->rom, 8) == 0)
                {
                    sensors[i]->alarm_flag = 1;     /*设置报警标志*/
                    sensors[i]->current_temp = ds18b20_get_temperature_rom(sensors[i]->rom);
                    if (sensors[i]->current_temp >= sensors[i]->high_threshold)
                    {
                        sensors[i]->high_ararm = 1; /*设置高温报警标志*/
                        sensors[i]->low_alarm = 0;
                    }
                    else if (sensors[i]->current_temp <= sensors[i]->low_threshold)
                    {
                        sensors[i]->low_alarm = 1;  /*设置低温报警标志*/
                        sensors[i]->high_ararm = 0;
                    }
                }
            }
            found++;
        }
    }while (search_result && found < DS18B20_COUNT);

    return found;
}
