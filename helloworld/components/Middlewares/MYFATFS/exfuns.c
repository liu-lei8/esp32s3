#include "exfuns.h"

#define FILE_MAX_TYPE_NUM   7       /*最多FILE_MAX_TYPE_NUM个大类*/
#define FILE_MAX_SUBT_NUM   7       /*最多FILE_MAX_SUBT_NUM个小类*/

static const char* FILE_TYPE_TBL[FILE_MAX_TYPE_NUM][FILE_MAX_SUBT_NUM] = {
    {"BIN"," "," "," "," "," "," "},                        /*二进制文件*/
    {"LRC"," "," "," "," "," "," "},                        /*歌词文件*/
    {"NES", "SMS"," "," "," "," "," "},                     /*复古游戏机的ROM文件*/
    {"TXT", "C", "H"," "," "," "," "},                      /*文本文件*/
    {"WAV", "MP3", "OGG", "FLAC", "AAC", "WMA", "MID"},     /*音乐文件*/
    {"DB", "DMP", "JPG", "JPEG", "GIF", "PNG"," "},         /*图片文件*/
    {"AVI"," "," "," "," "," "," "},                        /*视频文件*/
};

/*************************************************************** */
/*公共文件区，使用malloc的时候*/

/*逻辑磁盘工作区（在调用任何FATFS相关函数之前，必须先给fs申请内存）*/
FATFS* fs[FF_VOLUMES];

/*************************************************************** */

uint8_t exfuns_init(void)
{
    uint8_t i;

    for (i = 0; i < FF_VOLUMES; i++)
    {
        fs[i] = (FATFS*)malloc(sizeof(FATFS));      /*为磁盘i工作区申请内存*/
        if(!fs[i])break;
    }
    
    if (i == FF_VOLUMES)
    {
        return 0;
    }
    else    /*内存申请有一个失败，即失败*/
    {
        return 1;
    }
}

uint8_t exfuns_char_upper(uint8_t c)
{
    if (c < 'A')return c;   /*数字保持不变*/

    if (c > 'a')
    {
        return c - 0x20;    /*转换为大写字母*/
    }
    else
    {
        return c;           /*大写字母保持不变*/
    }
}

uint8_t exfuns_file_type(char* fname)
{
    uint8_t i = 0, j;
    char tbuf[5];
    char* suffix = NULL;
    uint8_t rval = 0;

    while (i < 250)
    {
        i++;
        if (*fname == '\0')break;       /*偏移到最后了*/
        fname++;
    }

    if (i == 250)return 0xFF;   /*错误的字符串*/

    for (i = 0; i < 5; i++)
    {
        fname--;
        if (*fname == '.')
        {
            fname++;
            suffix = fname;     /*指向文件后缀*/
            break;
        }
    }

    if (suffix == NULL) return 0xFF;
    
    for (i = 0; i < 4; i++)
    {
        tbuf[i] = *suffix++;
        tbuf[i] = exfuns_char_upper(tbuf[i]);   /*转换为大写字母*/
    }
    
    for (i = 0; i < FILE_MAX_TYPE_NUM; i++)
    {
        for (j = 0; j < FILE_MAX_SUBT_NUM; j++)
        {
            if (*FILE_TYPE_TBL[i][j] == ' ') break;     /*此组没有可以对比的成员了*/
            if (strcmp(tbuf, FILE_TYPE_TBL[i][j]) == 0)     /*找到文件类型了*/
            {
                return rval = (i << 4) | j;
            }
        }
    }
    
    return 0xFF;        /*没找到*/

}

uint8_t exfuns_get_free(uint8_t* pdrv, uint32_t* total, uint32_t* free)
{
    FATFS* fs1;
    uint8_t res;
    uint32_t free_clust = 0, free_sec = 0, total_sec = 0;

    /*得到磁盘信息以及空闲簇的数量*/
    res = f_getfree((const TCHAR*)pdrv, &free_clust, &fs1);

    if (res == FR_OK)
    {
        total_sec = (fs1->n_fatent - 2) * fs1->csize;   /*得到总扇区数量*/
        free_sec = free_clust * fs1->csize;             /*得到空闲扇区数量*/
#if FF_MAX_SS != 512    /*扇区大小不等于512字节，则转换为512字节大小*/
        total_sec *= fs1->ssize / 512;
        free_sec *= fs1->ssize / 512;
#endif
        *total = total_sec >> 1;    /*得到KB数，扇区数*512/1024，也就是直接除以2*/
        *free = free_sec >> 1;
    }

    return res;
}

uint8_t exfuns_file_copy(uint8_t(*fcpymsg)(uint8_t* pname, uint8_t pct, uint8_t mode), uint8_t* psrc, uint8_t* pdst, uint32_t totsize, uint32_t cpdsize, uint8_t fwmode)
{
    FIL* fsrc = NULL;
    FIL* fdst = NULL;
    uint8_t* fbuf = NULL;
    uint8_t res = 0;
    uint32_t lcpdsize = cpdsize;
    uint8_t curpct = 0;
    uint16_t br = 0, bw = 0;

    fsrc = (FIL*)malloc(sizeof(FIL));       /*给文件句柄申请内存*/
    fdst = (FIL*)malloc(sizeof(FIL));
    fbuf = (uint8_t*)malloc(8192);          /*每次读取或者写入文件的字节数*/

    if (fsrc == NULL || fdst == NULL || fbuf == NULL)
    {
        res = 100;      /*前面的值留给FATFS函数返回值*/
    }
    else    /*可以打开文件，更新百分比，并进行文件复制*/
    {
        if (fwmode == 0)fwmode = FA_CREATE_NEW; /*不覆盖，不存在文件则创建新的，存在则报错*/
        else fwmode = FA_CREATE_ALWAYS;         /*覆盖已存在文件*/

        /*打开只读文件*/
        res = f_open(fsrc, (const TCHAR*)psrc, FA_READ | FA_OPEN_EXISTING); 
        /*第一个文件打开成功，才打开第二个要写入的文件*/
        if (res == FR_OK) res = f_open(fdst, (const TCHAR*)pdst, FA_WRITE | fwmode);

        if (res == FR_OK)       /*两个文件都打开成功*/
        {
            if (totsize == 0)   /*仅复制单个文件*/
            {
                totsize = fsrc->obj.objsize;    /*此时总文件大小就等于当前文件大小*/
                lcpdsize = 0;
                curpct = 0;
            }
            else
            {
                curpct = (lcpdsize * 100) / totsize;    /*得到当前复制百分比*/
            }
            fcpymsg(psrc, curpct, 0x02);    /*更新百分比*/

            while (res == FR_OK)    /*开始复制*/
            {
                res = f_read(fsrc, fbuf, 8192, &br);    /*一次从源文件读8KB*/
                if (res != FR_OK || br == 0) break; /*错误/读到源文件末尾结束复制*/

                res = f_write(fdst, fbuf, br, &bw);     /*将源文件的数据写入目标文件*/
                lcpdsize += bw;

                if (curpct != (lcpdsize * 100) / totsize)   /*更新当前百分比*/
                {
                    curpct = (lcpdsize * 100) / totsize;
                    if (fcpymsg(psrc, curpct, 0x02))    /*更新显示百分比*/
                    {
                        res = 0XFF;                     /*强制退出*/
                        break;
                    }
                }

                if (res || bw < br) break;  /*错误/内存写满了则退出*/
            }

            f_close(fsrc);
            f_close(fdst);
        }
    }

    free(fsrc);
    free(fdst);
    free(fbuf);

    return res;
}

uint8_t* exfuns_get_src_dname(uint8_t* pname)
{
    uint16_t count = 0;

    while (*pname != 0)
    {
        pname++;
        count++;
    }

    if (count < 4) return 0;

    while ((*pname != '/') && (*pname != '\\')) pname--;    /*追溯到倒数第一个"/"或"\"处*/

    return ++pname;
}

uint32_t exfuns_get_folder_size(uint8_t* fdname)
{
#define MAX_PATHNAME_DEPTH  512 + 1     /*最大目标文件路径+文件名深度*/

    uint8_t res = 0;
    FF_DIR* fdir = 0;           /*目录*/
    FILINFO* finfo = 0;         /*文件信息*/
    uint8_t* pathname = 0;      /*目标文件夹路径*/
    uint16_t pathlen = 0;       /*路径长度*/
    uint32_t fdsize = 0;        /*文件夹总大小*/

    fdir = (FF_DIR*)malloc(sizeof(FF_DIR));         /*申请内存*/
    finfo = (FILINFO*)malloc(sizeof(FILINFO));

    if (fdir == NULL || finfo == NULL) res = 100;

    if (res == 0)   /*内存申请没有问题*/
    {
        pathname = malloc(MAX_PATHNAME_DEPTH);
        if (pathname == NULL) res = 101;

        if (res == 0)
        {
            pathname[0] = 0;
            strcat((char*)pathname, (const char*)fdname);   /*复制路径*/
            res = f_opendir(fdir, (const TCHAR*)fdname);    /*打开源目录*/

            if (res == 0)   /*打开成功*/
            {
                while (1)   /*开始统计文件夹里所有文件大小（包括子目录）*/
                {
                    res = f_readdir(fdir, finfo);   /*开始读取目录里的一个文件*/
                    if (res != FR_OK || finfo->fname[0] == 0) break;    /*错误或到末尾*/
                    if (finfo->fname[0] == '.') continue;/*忽略"."本级目录和".."上级目录*/

                    if (finfo->fattrib & 0x10)  /*文件属性（0x10子目录，0x20归属文件）*/
                    {
                        pathlen = strlen((const char*)pathname);    /*得到当前路径长度*/
                        strcat((char*)pathname, (const char*)"/");   /*加斜杠*/
                        strcat((char*)pathname, (const char*)finfo->fname); /*加子目录名*/
                        fdsize += exfuns_get_folder_size(pathname); /*得到子目录大小，递归*/
                        pathname[pathlen] = 0;                      /*加入结束符*/
                    }
                    else    /*非目录，直接加上文件大小*/
                    {
                        fdsize += finfo->fsize;
                    }
                }
            }
            free(pathname);
        }
    }

    free(fdir);
    free(finfo);

    return fdsize;
}

uint8_t exfuns_folder_copy(uint8_t(*fcpymsg)(uint8_t* pname, uint8_t pct, uint8_t mode), uint8_t* psrc, uint8_t* pdst, uint32_t* totsize, uint32_t* cpdsize, uint8_t fwmode)
{
#define MAX_PATHNAME_DEPTH  512 + 1     /*最大目标路径+文件名深度*/

    uint8_t res = 0;
    FF_DIR* srcdir = 0;     /*源目录句柄*/
    FF_DIR* dstdir = 0;     /*目标目录句柄*/
    FILINFO* finfo = 0;     /*文件信息*/
    uint8_t* fn = 0;        /*末尾文件夹名/文件名*/

    uint8_t* dstpathname = 0;   /*目标文件夹路径+文件名*/
    uint8_t* srcpathname = 0;   /*源文件夹路径+文件名*/
    
    uint16_t dstpathlen = 0;    /*目标路径长度*/
    uint16_t srcpathlen = 0;    /*源路径长度*/

    srcdir = (FF_DIR*)malloc(sizeof(FF_DIR));
    dstdir = (FF_DIR*)malloc(sizeof(FF_DIR));
    finfo = (FILINFO*)malloc(sizeof(FILINFO));

    if (srcdir == NULL || dstdir == NULL || finfo == NULL) res = 100;

    if (*totsize == 0)
    {
        *totsize = exfuns_get_folder_size(psrc);
        *cpdsize = 0;
    }

    if (res == 0)
    {
        dstpathname = malloc(MAX_PATHNAME_DEPTH);
        srcpathname = malloc(MAX_PATHNAME_DEPTH);
        if (dstpathname == NULL || srcpathname == NULL) res = 101;

        if (res == 0)
        {
            dstpathname[0] = 0;
            srcpathname[0] = 0;
            strcat((char*)dstpathname, (const char*)pdst);  /*复制原始目标文件夹路径*/
            strcat((char*)srcpathname, (const char*)psrc);  /*复制原始源文件夹路径*/
            res = f_opendir(srcdir, (const TCHAR*)psrc);    /*打开源目录*/

            if (res == 0)   /*成功打开目录*/
            {
                strcat((char*)dstpathname, "/");    /*目标路径加斜杠*/
                fn = exfuns_get_src_dname(psrc);    /*获取源文件夹名*/

                if (fn == 0)    /*卷标的拷贝*/
                {
                    dstpathlen = strlen((const char*)dstpathname);
                    dstpathname[dstpathlen] = psrc[0];      /*记录卷标*/
                    dstpathname[dstpathlen + 1] = 0;        /*结束符*/
                }
                else            /*加文件夹名*/
                {
                    strcat((char*)dstpathname, (const char*)fn);
                }

                fcpymsg(fn, 0, 0x04);       /*更新文件夹名*/
                res = f_mkdir((const char*)dstpathname);/*若文件夹不存在则创建新的，否则不创*/

                if (res == FR_EXIST) res = 0;   /*文件夹已经存在的情况*/

                while (res == 0)       /*开始复制文件夹里的文件或子目录*/
                {
                    res = f_readdir(srcdir, finfo);    /*读取文件夹下的一个目录*/
                    if (res != FR_OK || finfo->fname[0] == 0) break;/*错误或者到末尾结束*/
                    if (finfo->fname[0] == '.') continue;   /*本目录或上级目录跳过*/

                    fn = (uint8_t*)finfo->fname;    /*得到文件名或者子目录名*/
                    dstpathlen = strlen((const char*)dstpathname);/*得到当前目标路径长度*/
                    srcpathlen = strlen((const char*)srcpathname);/*得到源目录路径长度*/

                    strcat((char*)srcpathname, "/");    /*源目录路径加斜杠*/
                    if (finfo->fattrib & 0x10)  /*文件属性(0x10子目录，0x20归档文件)*/
                    {
                        strcat((char*)srcpathname, (const char*)fn);    /*加子目录名*/
                        res = exfuns_folder_copy(fcpymsg, srcpathname, dstpathname, totsize, cpdsize, fwmode);    /*复制文件夹，递归*/
                    }
                    else    /*直接复制源目录里的文件*/
                    {
                        strcat((char*)dstpathname, (const char*)"/");   /*加斜杠*/
                        strcat((char*)dstpathname, (const char*)fn);    /*加文件名*/
                        strcat((char*)srcpathname, (const char*)fn);    /*加文件名*/
                        res = exfuns_file_copy(fcpymsg, srcpathname, dstpathname, *totsize, *cpdsize, fwmode);    /*复制文件*/
                        *cpdsize += finfo->fsize;   /*增加一个文件的大小*/
                    }

                    srcpathname[srcpathlen] = 0;    /*加结束符*/
                    dstpathname[dstpathlen] = 0;
                }
            }
            free(srcpathname);
            free(dstpathname);
        }
    }

    free(srcdir);
    free(dstdir);
    free(finfo);

    return res;
}

uint8_t fcpymsg(uint8_t* pname, uint8_t pct, uint8_t mode)
{
    char disp_buf[50];

    /*更新文件名*/
    if (mode & 0x01)
    {
        snprintf(disp_buf, sizeof(disp_buf), "File:%s",(char*)pname);
        lcd_show_string(30, 160, disp_buf, RED, WHITE, 16, 0);
    }

    /*更新百分比*/
    if (mode & 0x02)
    {
        snprintf(disp_buf, sizeof(disp_buf), "Process:%d%%",pct);
        lcd_show_string(30, 180, disp_buf, RED, WHITE, 16, 0);
    }

    /*更新文件夹名*/
    if (mode & 0x04)
    {
        snprintf(disp_buf, sizeof(disp_buf), "Folder:%s",(char*)pname);
        lcd_show_string(30, 140, disp_buf, RED, WHITE, 16, 0);
    }

    if (key_scan(0) == BOOT_PRES)
    {
        lcd_show_string(30, 200, "Copy aborted!", RED, WHITE, 16, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
        return 1;   /*返回1表示强制退出*/
    }

    return 0;   /*正常*/
}
