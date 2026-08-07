#include "audioplay.h"

_audiodev g_audiodev;

void audio_start(void)
{
    g_audiodev.status = 3 << 0;
    i2s_trx_start();
}

void audio_stop(void)
{
    g_audiodev.status = 0;
    i2s_trx_stop();
}

/**
 * @brief 根据文件夹路径获取对应音频文件的数量
 * @param path：文件夹路径
 * @retval 返回音频文件的数量
 */
static uint16_t audio_get_tnum(uint8_t* path)
{
    uint8_t res;
    uint16_t rval = 0;
    FF_DIR tdir;
    FILINFO* tfileinfo;

    tfileinfo = (FILINFO*)malloc(sizeof(FILINFO));
    res = f_opendir(&tdir, (const TCHAR*)path);

    if (tfileinfo && res == FR_OK)
    {
        while (1)
        {
            res = f_readdir(&tdir, tfileinfo);
            if (res != FR_OK || tfileinfo->fname[0] == 0)
            {
                break;
            }
            
            res = exfuns_file_type(tfileinfo->fname);
            printf("File: %s, type: 0x%02X\n", tfileinfo->fname, res);
            if ((res & 0xF0) == 0x40)   /*是音频文件*/
            {
                rval++;
            }
        }
    }

    free(tfileinfo);

    return rval;
}

void audio_meg_show(uint32_t totsec, uint32_t cursec, uint32_t bitrate)
{
    static uint16_t playtime = 0xFFFF;

    if (playtime != cursec)
    {
        playtime = cursec;

        lcd_show_num(30, 210, playtime / 60, 2, RED, WHITE, 16, 0);         /*显示当前播放的时间*/
        lcd_show_char(30 + 16, 210, ':', RED, WHITE, 16, 1);
        lcd_show_num(30 + 24, 210, playtime % 60, 2, RED, WHITE, 16, 0);

        lcd_show_char(30 + 40, 210, '/', RED, WHITE, 16, 1);

        lcd_show_num(30 + 48, 210, totsec / 60, 2, RED, WHITE, 16, 0);      /*显示歌曲总时间*/
        lcd_show_char(30 + 64, 210, ':', RED, WHITE, 16, 1);
        lcd_show_num(30 + 72, 210, totsec % 60, 2, RED, WHITE, 16, 0);

        lcd_show_num(30 + 110, 210, bitrate / 1000, 4, RED, WHITE, 16, 0);  /*显示每一千秒的传输位数*/
        lcd_show_string(30 + 142, 210, "Kbps", RED, WHITE, 16, 1);
    }
}

/**
 * @brief 在spilcd上显示当前播放第几个音频和音频总数
 * @param index：正在播放第几个音频
 * @param total：音频总数
 */
static void audio_index_show(uint16_t index, uint16_t total)
{
    lcd_show_num(30, 230, index, 3, RED, WHITE, 16, 0);
    lcd_show_char(30 + 24, 230, '/',RED, WHITE, 16, 1);
    lcd_show_num(30 + 32, 230, total, 3, RED, WHITE, 16, 0);
}

/**
 * @brief 将簇号转换为簇的起始扇区号
 * @param fs:文件系统对象
 * @param clst:要转换的簇号，簇号从0开始计算
 * @retval 返回簇的起始扇区号
 */
static LBA_t audio_clst2sect(FATFS* fs, DWORD clst)
{
    clst -= 2;

    if (clst >= fs->n_fatent - 2)
    {
        return 0;
    }

    return fs->database + (DWORD)fs->csize * clst;
}

/**
 * @brief 在目录中偏移找到对应的目录项
 * @param dp：目录指针
 * @param ofs：偏移字节
 * @return FR_OK:偏移成功，其他：偏移失败
 */
static FRESULT audio_dir_sdi(FF_DIR* dp, DWORD ofs)
{
    DWORD clst;
    FATFS* fs;

    clst = dp->obj.sclust;
    fs = dp->obj.fs;

    if (ofs >= (DWORD)((FF_FS_EXFAT && fs->fs_type == FS_EXFAT) ? 0X10000000 : 0X200000) || ofs % 32)
    {
        return FR_INT_ERR;
    }

    if (clst == 0 && fs->fs_type >= FS_FAT32)
    {
        clst = fs->dirbase;     /*获取实际的根目录的起始簇号*/

        if (FF_FS_EXFAT)
        {
            dp->obj.stat = 0;
        }
    }

    if (clst == 0)                  /*FS_FAT12,FS_FAT16,FS_FAT24的系统*/
    {
        if (ofs / 32 >= fs->n_rootdir)
        {
            return FR_INT_ERR;
        }

        dp->sect = fs->dirbase;     /*获取起始扇区*/
    }
    else                            /*FS_FAT32,FS_EXFAT的系统*/
    {
        dp->sect = audio_clst2sect(fs, clst);
    }

    if (dp->sect == 0)
    {
        return FR_INT_ERR;
    }

    dp->clust = clst;               /*设置当前根目录起始簇*/
    dp->dptr = ofs;                 /*设置当前偏移字节*/
    dp->sect += ofs / fs->ssize;           /*设置偏移的扇区*/
    dp->dir = fs->win + ofs % fs->ssize;   /*在win工作区找到对应的条目*/

    return FR_OK;
}

/**
 * @brief 播放不同音频文件
 * @param fname：文件路径+文件名
 * @retval 按键值
 *  @arg   KEY0_PRES:下一首
 *  @arg   KEY1_PRES:上一首
 *  @arg   其他：错误
 */
static uint8_t audio_play_song(uint8_t* fname)
{
    uint8_t res;

    res = exfuns_file_type((char*)fname);

    switch(res)
    {
        case T_WAV:
            res = wav_play_song(fname);
            break;
        case T_MP3:
            /*自行实现*/
            break;
        
        default:        /*其他文件，自动播放下一首*/
            printf("can't play %s\r\n", fname);
            res = KEY0_PRES;
            break;
    }

    return res;
}

void audio_play(void)
{
    FF_DIR tdir;
    uint16_t wavtotnum;
    FILINFO* wavfileinfo;
    uint8_t* pname;
    uint32_t* wavoffsettable;
    uint16_t curindex = 0;
    uint32_t temp;
    uint8_t key;
    uint8_t res;

    while (f_opendir(&tdir, "0:/MUSIC"))
    {
        text_show_string(30, 190, 240, 16, "打开MUSIC文件夹错误！", 16, 0, BLUE, WHITE);
        vTaskDelay(200);
        lcd_fill(30, 190, 240, 206, WHITE);
        vTaskDelay(200);
    }
    
    wavtotnum = audio_get_tnum((uint8_t*)"0:/MUSIC");      /*得到总有效WAV文件数*/

    while (wavtotnum == 0)
    {
        text_show_string(30, 190, 240, 16, "没有音乐文件！", 16, 0, BLUE, WHITE);
        vTaskDelay(200);
        lcd_fill(30, 190, 240, 206, WHITE);
        vTaskDelay(200);
    }

    wavfileinfo = (FILINFO*)malloc(sizeof(FILINFO));
    pname  = malloc(255 * 2 + 1);
    wavoffsettable = (uint32_t*)malloc(4 * wavtotnum);

    while (!wavfileinfo || !pname || !wavoffsettable)
    {
        text_show_string(30, 190, 240, 16, "内存分配失败！", 16, 0, BLUE, WHITE);
        vTaskDelay(200);
        lcd_fill(30, 190, 240, 206, WHITE);
        vTaskDelay(200);
    }

    res = f_opendir(&tdir, "0:/MUSIC");

    if (res == FR_OK)
    {
        curindex = 0;

        while (1)
        {
            temp = tdir.dptr;       /*获取当前偏移的字节*/
            
            res = f_readdir(&tdir, wavfileinfo);
            if (res == FR_OK && wavfileinfo->fname[0] != 0)
            {
                res = exfuns_file_type(wavfileinfo->fname);
                if ((res & 0xF0) == 0x40)
                {
                    wavoffsettable[curindex] = temp;        /*将WAV在目录中的偏移字节记录下来*/
                    curindex++;
                }
            }
            else
            {
                break;
            }
        }
    }

    curindex = 0;
    res = f_opendir(&tdir, "0:/MUSIC");

    while (res == FR_OK)
    {
        audio_dir_sdi(&tdir, wavoffsettable[curindex]);     /*改变当前的目录索引*/
        res = f_readdir(&tdir, wavfileinfo);                /*读取对应的目录项*/
        if (res != FR_OK || wavfileinfo->fname[0] == 0)
        {
            break;
        }

        pname[0] = '\0';
        strcat((char*)pname, "0:/MUSIC/");                  /*这里注意必须把pname先清零，因为分配内存后内容是随机的*/
        strcat((char*)pname, wavfileinfo->fname);
        lcd_fill(30, 190, 240, 206, WHITE);
        text_show_string(30, 190, 240, 16, wavfileinfo->fname, 16, 0, RED, WHITE);
        audio_index_show(curindex + 1, wavtotnum);
        key = audio_play_song(pname);

        if (key == KEY0_PRES)       /*下一首*/
        {
            curindex++;
            if (curindex >= wavtotnum)
            {
                curindex = 0;
            }
        }
        else if (key == KEY1_PRES)  /*上一首*/
        {
            if (curindex == 0)
            {
                curindex = wavtotnum - 1;
            }
            else
            {
                curindex--;
            }
        }
        else break;
    }

    free(wavfileinfo);
    free(wavoffsettable);
    free(pname);
}
