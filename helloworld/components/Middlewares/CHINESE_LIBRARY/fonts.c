#include "fonts.h"

_font_info ftinfo;

static const char* fonts_tag = "fonts";
const esp_partition_t* storage_partition;
uint8_t* unigbk_table = NULL;   /*存储UNIGBK转换表指针*/

/*字库存放在磁盘中的路径*/
const char* FONT_GBK_PATH[4] = {
    "/SYSTEM/FONT/UNIGBK.BIN",
    "/SYSTEM/FONT/GBK12.FON",
    "/SYSTEM/FONT/GBK16.FON",
    "/SYSTEM/FONT/GBK24.FON",
};

/*更新时的提示信息*/
const char* FONT_UPDATE_REMIND_TAL[4] = {
    "Updating UNIGBK.BIN",
    "Updating GBK12.FON",
    "Updating GBK16.FON",
    "Updating GBK24.FON",
};

esp_err_t fonts_partition_read(uint32_t offset, void* buffer, uint32_t lenght)
{
    esp_err_t err;

    if (buffer == NULL || lenght > SECTOR_SIZE)
    {
        ESP_LOGE(fonts_tag, "ESP_ERR_INVALID_ARG");
        return ESP_ERR_INVALID_ARG;
    }

    err = esp_partition_read(storage_partition, offset, buffer, lenght);

    if (err != ESP_OK)
    {
        ESP_LOGE(fonts_tag, "Flash read failed");
    }

    return err;
}

esp_err_t fonts_partition_write(uint32_t offset, void* buffer, uint32_t lenght)
{
    esp_err_t err;

    if (buffer == NULL || lenght > SECTOR_SIZE)
    {
        ESP_LOGE(fonts_tag, "ESP_ERR_INVALID_ARG");
        return ESP_ERR_INVALID_ARG;
    }

    err = esp_partition_write(storage_partition, offset, buffer, lenght);

    if (err != ESP_OK)
    {
        ESP_LOGE(fonts_tag, "Flash write failed");
    }

    return err;
}

esp_err_t fonts_partition_erase_sector(uint32_t offset)
{
    esp_err_t err;

    err = esp_partition_erase_range(storage_partition, offset, SECTOR_SIZE);

    if (err != ESP_OK)
    {
        ESP_LOGE(fonts_tag, "Flash erase failed");
    }

    return err;
}

void fonts_progress_show(uint16_t x, uint16_t y, uint8_t size, uint32_t totsize, uint32_t pos, uint16_t fc, uint16_t bc)
{
    float prog;
    uint8_t t = 0xFF;

    prog = (float)pos / totsize;
    prog *= 100;

    t = prog;
    if (t > 100) t = 100;

    lcd_show_num(x, y, t, 3, fc, bc, size, 0);
    lcd_show_char(x + 3 * size / 2, y, '%', fc, bc, size, 0);
}

/**
 * @brief 更新某个字体文件
 * @param xy:lcd显示坐标
 * @param size:lcd显示字体大小
 * @param fpath:字体文件的来源路径
 * @param fx:要更新的内容
 *  @arg    0:UNIGBK,
 *  @arg    1:GBK12,
 *  @arg    2:GBK16,
 *  @arg    3:GBK24,
 * @param fc:lcd字体颜色
 * @param bc:lcd字体背景颜色
 * @return 0,成功；其他，错误代码
 */
static uint8_t fonts_update_fontx(uint16_t x, uint16_t y, uint8_t size, uint8_t* fpath, uint8_t fx, uint16_t fc, uint16_t bc)
{
    FIL* fftemp;
    uint8_t* tempbuf;
    uint8_t res;
    uint8_t rval = 0;
    uint32_t flash_addr = 0;
    uint16_t bread;             /*bytes read实际读到的字节数*/
    uint32_t offx = 0;

    fftemp = (FIL*)malloc(sizeof(FIL));
    if (fftemp == NULL) rval = 1;

    tempbuf = malloc(4096);         /*分配4096字节*/
    if (tempbuf == NULL) rval = 1;

    res = f_open(fftemp, (const TCHAR*)fpath, FA_READ | FA_OPEN_EXISTING);
    if (res) rval = 2;  /*打开文件失败*/

    if (rval == 0)
    {
        switch (fx)
        {
            case 0:                                                 /*更新UNIGBK.BIN文件*/
                ftinfo.ugbkaddr = FONTINFOADDR + sizeof(ftinfo);    /*信息头之后，紧跟UNIGBK转换码表*/
                ftinfo.ugbksize = fftemp->obj.objsize;              /*UNIGBK文件大小*/
                flash_addr = ftinfo.ugbkaddr;                       /*UNIGBK起始地址*/
                break;

            case 1:                                                 /*更新GBK12.FON文件*/
                ftinfo.f12addr = ftinfo.ugbkaddr + ftinfo.ugbksize; /*UNIGBK之后紧跟GBK12*/
                ftinfo.gbk12size = fftemp->obj.objsize;             /*GBK12字库大小*/
                flash_addr = ftinfo.f12addr;                        /*GBK12起始地址*/
                break;

            case 2:                                                 /*更新GBK16.FON文件*/
                ftinfo.f16addr = ftinfo.f12addr + ftinfo.gbk12size; /*GBK12之后紧跟GBK16*/
                ftinfo.gbk16size = fftemp->obj.objsize;             /*GBK16字库大小*/
                flash_addr = ftinfo.f16addr;                        /*GBK16起始地址*/
                break;

            case 3:                                                 /*更新GBK24.FON文件*/
                ftinfo.f24addr = ftinfo.f16addr + ftinfo.gbk16size; /*GBK16之后紧跟GBK24*/
                ftinfo.gbk24size = fftemp->obj.objsize;             /*GBK24字库大小*/
                flash_addr = ftinfo.f24addr;                        /*GBK24起始地址*/
                break;

            default:break;
        }

        while (res == FR_OK)        /*执行死循环,从SD卡读取字库数据写入分区表*/
        {
            res = f_read(fftemp, tempbuf, 4096, (UINT)&bread);  /*读取数据*/
            if (res != FR_OK) break;    /*执行错误*/

            /*从0开始写入bread个数据*/
            fonts_partition_write(flash_addr + offx, tempbuf, bread);
            offx += bread;
            fonts_progress_show(x, y, size, fftemp->obj.objsize, offx, fc, bc);/*显示进度*/

            if (bread != 4096) break;   /*读完了*/
        }

        f_close(fftemp);
    }

    free(fftemp);
    free(tempbuf);

    return rval;
}

uint8_t fonts_update_font(uint16_t x, uint16_t y, uint8_t size, uint8_t* src, uint16_t fc, uint16_t bc)
{
    uint8_t* pname;
    uint32_t* buf;
    FIL* fftemp;
    uint16_t i, j;
    uint8_t res = 0;
    uint8_t rval = 0;

    pname = malloc(100);
    buf = malloc(4096);
    fftemp = (FIL*)malloc(sizeof(FIL));

    if (pname == NULL || buf == NULL || fftemp == NULL)
    {
        free(pname);
        free(buf);
        free(fftemp);
        return 5;       /*内存申请失败*/
    }

    for (i = 0; i < 4; i++)     /*先查找文件UNIGBK,GBK12,GBK16,GBK24是否正常*/
    {
        strcpy((char*)pname, (const char*)src);                 /*加磁盘路径*/
        strcat((char*)pname, (const char*)FONT_GBK_PATH[i]);    /*追加具体文件路径*/
        res = f_open(fftemp, (const TCHAR*)pname, FA_READ | FA_OPEN_EXISTING);/*尝试打开*/

        if (res)
        {
            rval |= 1 << 7;     /*标记打开文件失败*/
            break;
        }
    }
    free(fftemp);

    if (rval == 0)  /*字库文件都存在*/
    {
        lcd_show_string(x, y, "Erasing sectors...", fc, bc, size, 0);   /*提示正在擦除扇区*/

        for (i = 0; i < FONTSECSIZE; i++)   /*先擦除要存放字库的扇区，才能写入数据*/
        {
            /*显示进度*/
            fonts_progress_show(x + 20 * size / 2, y, size, FONTSECSIZE, i, fc, bc);
            /*读出整个扇区的内容*/
            fonts_partition_read(((FONTINFOADDR / 4096) + i) * 4096, (uint8_t*)buf, 4096);

            for (j = 0; j < 1024; j++)  /*校验数据,看一整个扇区中的数据是否为空，都为1为空*/
            {
                if (buf[j] != 0xFFFFFFFF) break;    /*需要擦除*/
            }

            if (j != 1024)
            {
                /*擦除扇区*/
                fonts_partition_erase_sector(((FONTINFOADDR / 4096) + i) * 4096);
            }
        }

        for (i = 0; i < 4; i++) /*依次更新UNIGBK,GBK12,GBK16,GBK24*/
        {
            lcd_show_string(x, y, FONT_UPDATE_REMIND_TAL[i], fc, bc, size, 0);
            strcpy((char*)pname, (const char*)src);                 /*复制磁盘路径*/
            strcat((char*)pname, (const char*)FONT_GBK_PATH[i]);    /*追加具体文件路径*/
            res = fonts_update_fontx(x + 20 * size / 2, y, size, pname, i, fc, bc);/*更新字库*/

            if (res)
            {
                free(buf);
                free(pname);
                return 1 + i;
            }
        }
        
        ftinfo.fontok = 0xAA;   /*全部更新好了*/
        fonts_partition_write(FONTINFOADDR, &ftinfo, sizeof(ftinfo));   /*保存字库信息*/
    }

    free(pname);
    free(buf);
    return rval;
}

uint8_t fonts_init(void)
{
    uint8_t t = 0;

    storage_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");

    if (storage_partition == NULL)
    {
        ESP_LOGE(fonts_tag, "Flash 'storage' partition not found");
        return 1;
    }

    while (t < 10)  /*连续读取十次都是错误，说明确实有问题，得更新分区表字库了*/
    {
        t++;
        fonts_partition_read(FONTINFOADDR, &ftinfo, sizeof(ftinfo));

        if (ftinfo.fontok == 0xAA)
        {
            break;
        }
        vTaskDelay(10);
    }

    if (ftinfo.fontok != 0xAA)
    {
        return 1;
    }

    // --- 新增：分扇区加载 UNIGBK 表到 RAM ---
    if (unigbk_table == NULL) {
        unigbk_table = (uint8_t*)malloc(ftinfo.ugbksize);
        if (unigbk_table == NULL) {
            ESP_LOGE(fonts_tag, "Malloc unigbk_table failed");
            return 2;
        }

        uint32_t remain = ftinfo.ugbksize;
        uint32_t offset = 0;
        uint8_t *buf_ptr = unigbk_table;
        uint32_t addr = ftinfo.ugbkaddr;

        while (remain > 0) {
            uint32_t chunk = (remain > SECTOR_SIZE) ? SECTOR_SIZE : remain;
            esp_err_t err = fonts_partition_read(addr + offset, buf_ptr, chunk);
            if (err != ESP_OK) {
                ESP_LOGE(fonts_tag, "Read UNIGBK sector failed at offset %lu", offset);
                free(unigbk_table);
                unigbk_table = NULL;
                return 2;
            }
            offset += chunk;
            buf_ptr += chunk;
            remain -= chunk;
        }
    }
    // --- 加载完毕 ---

    return 0;
}

