#include "videoplay.h"

const char* videoplay = "videoplay";
extern uint8_t frameup;
extern int Windows_Width;
extern int Windows_Height;
extern MessageBufferHandle_t audio_mbox;

/**
 * @brief 获取指定路径下的有效视频文件
 * @param path：文件路径
 * @retval 0：错误或没有视频文件；其他值：有效视频文件数量
 */
static uint16_t video_get_tnum(char* path)
{
    uint8_t res;
    uint16_t rval = 0;
    FF_DIR tdir;
    FILINFO* tfileinfo;

    tfileinfo = (FILINFO*)malloc(sizeof(FILINFO));              /* 申请内存 */
    res = (uint8_t)f_opendir(&tdir, (const TCHAR*)path);         /* 打开目录 */

    if ((res == 0) && tfileinfo)
    {
        while (1)
        {
            res = (uint8_t)f_readdir(&tdir, tfileinfo);          /* 读取目录下的一个文件*/

            if (res != 0 || tfileinfo->fname[0] == 0)           /* 读取错误或者读到末尾 */
            {
                break;
            }

            res = exfuns_file_type(tfileinfo->fname);

            if ((res & 0xF0) == 0x60)                           /* 是视频文件 */
            {
                rval++;
            }
        }
    }

    f_closedir(&tdir);
    free(tfileinfo);
    return rval;
}

/**
 * @brief spilcd上显示音视频文件的基本信息
 * @param name：要显示的文件名
 * @param index：当前播放的第几个视频文件
 * @param total：总视频文件
 */
static void video_bmsg_show(uint8_t* name, uint16_t index, uint16_t total)
{
    uint8_t* buf = malloc(100);

    sprintf((char*)buf, "文件名:%s", name);
    text_show_string(10, 10, lcd_self.width - 10, 16, (char*)buf, 16, 1, RED, WHITE);

    sprintf((char*)buf, "索引:%d/%d", index, total);
    text_show_string(10, 30, lcd_self.width - 10, 16, (char*)buf, 16, 1, RED, WHITE);

    free(buf);
}

/**
 * @brief spilcd上显示音视频文件的相关信息
 * @param aviinfo:音视频文件信息结构体指针
 */
static void video_info_show(AVI_INFO* aviinfo)
{
    uint8_t* buf = malloc(100);

    sprintf((char*)buf, "声道数:%d, 采样率:%ld", aviinfo->Channels, aviinfo->SampleRate);
    text_show_string(10, 50, lcd_self.width - 10, 16, (char*)buf, 16, 1, RED, WHITE);

    sprintf((char*)buf, "帧率:%ld帧", 1000 / (aviinfo->SecPerFrame / 1000));
    text_show_string(10, 70, lcd_self.width - 10, 16, (char*)buf, 16, 1, RED, WHITE);

    free(buf);
}

/**
 * @brief spilcd上显示视频文件播放时间信息
 * @param favi：当前播放的视频文件
 * @param aviinfo：avi控制结构体
 */
static void video_time_show(FIL* favi, AVI_INFO* aviinfo)
{
    static uint32_t oldsec = 0;         /* 上一次播放的时间 */

    uint8_t* buf;

    uint32_t totsec;                /* 视频总时长 */
    uint32_t cursec;                /* 当前播放时长 */

    totsec = (aviinfo->SecPerFrame / 1000) * aviinfo->TotalFrame;   /* 单位毫秒ms */
    totsec /= 1000;                                                 /* 单位秒s*/
    cursec = ((double)favi->fptr / favi->obj.objsize) * totsec;     /* 获取当前播放到多少秒 */

    if (oldsec != cursec)           /* 更新显示时间 */
    {
        buf = malloc(100);
        oldsec = cursec;

        sprintf((char*)buf, "Play time:%02ld:%02ld:%02ld/%02ld:%02ld:%02ld", cursec / 3600, (cursec % 3600) / 60, cursec % 60, totsec / 3600, (totsec % 3600) / 60, totsec % 60);
        text_show_string(10, 90, lcd_self.width - 10, 16, (char*)buf, 16, 0, RED, WHITE);

        free(buf);
    }
}

/**
 * @brief AVI文件查找，视频快进或快退5秒
 * @param favi：文件对象
 * @param aviinfo：AVI文件信息结构体
 * @param mbuf：输出缓冲区
 * @retval 0:成功；其他：失败
 */
static uint8_t video_seek(FIL* favi, AVI_INFO* aviinfo, uint8_t* mbuf)
{
    uint32_t totsec;
    uint32_t delta;
    uint32_t fpos = favi->fptr;                 /* 获取当前文件位置 */
    UINT br;
    uint8_t* pbuf;
    uint16_t offset;
    uint8_t key;

    totsec = (aviinfo->SecPerFrame / 1000) * aviinfo->TotalFrame;
    totsec /= 1000;                             /* 秒钟数*/
    delta = (favi->obj.objsize / totsec) * 5;   /* 每5秒前进的数据量 */

    while (1)
    {
        key = xl9555_key_scan(1);               /*支持连续按下*/

        if (key == KEY2_PRES)                   /* 前进5秒 */
        {
            if (fpos < favi->obj.objsize)
            {
                fpos += delta;
            }

            if (fpos > (favi->obj.objsize - AVI_VIDEO_BUF_SIZE))
            {
                fpos = favi->obj.objsize - AVI_VIDEO_BUF_SIZE;
            }
        }
        else if (key == KEY3_PRES)              /* 倒退5秒 */
        {
            if (fpos > delta)
            {
                fpos -= delta;
            }
            else
            {
                fpos = 0;
            }
        }
        else break;

        f_lseek(favi, fpos);
        f_read(favi, mbuf, AVI_VIDEO_BUF_SIZE, &br);
        pbuf = mbuf;

        if (fpos == 0)                          /* 从0开始，得先寻找movi ID*/
        {
            offset = avi_search_id(pbuf, AVI_VIDEO_BUF_SIZE, "movi");
        }
        else offset = 0;

        offset += avi_search_id(pbuf + offset, AVI_VIDEO_BUF_SIZE, g_avix.VideoFLAG);   /*寻找视频帧*/
        avi_get_streaminfo(pbuf + offset);      /* 获取视频帧的ID信息 */
        f_lseek(favi, fpos + offset + 8);       /* 跳过ID信息，直接偏移到数据流的开始处 */

        if (g_avix.StreamID == AVI_VIDS_FLAG)
        {
            f_read(favi, mbuf, g_avix.StreamSize + 8, &br);        /* 读取整帧+下一帧ID信息 */
            mjpegdec_decode(mbuf, g_avix.StreamSize);           /* 开始解码显示视频帧 */
        }
        else
        {
            ESP_LOGE(videoplay, "Error Stream Flag!");
            return 1;
        }

        video_time_show(favi, &g_avix);
    }

    return 0;
}

/**
 * @brief 播放mjpeg压缩格式的AVI视频文件
 * @param pname: 文件路径+文件名
 * @retval: 返回按键值
 *  @arg    KEY0_PRES: 下一个视频
 *  @arg    KEY1_PRES: 上一个视频
 *  @arg    其他值    : 错误代码
 */
static uint8_t video_play_mjpeg(uint8_t* pname)
{
    uint8_t* framebuf;
    uint8_t* pbuf;
    FIL* favi;
    uint8_t res = 0;
    UINT br;
    uint32_t offset = 0;
    uint8_t key;
    uint8_t temp;

    framebuf = malloc(AVI_VIDEO_BUF_SIZE);
    favi = (FIL*)malloc(sizeof(FIL));

    if (framebuf == NULL || favi == NULL)
    {
        ESP_LOGE(videoplay, "Memory Error!");
        res = 0xFF;
    }

    memset(framebuf, 0, AVI_VIDEO_BUF_SIZE);

    if (res == 0)
    {
        res = (uint8_t)f_open(favi, (const TCHAR*)pname, FA_READ);          /* 打开文件 */

        if (res == 0)
        {
            pbuf = framebuf;
            res = (uint8_t)f_read(favi, pbuf, AVI_VIDEO_BUF_SIZE, &br);     /* 开始读取 */

            if (res != 0)
            {
                ESP_LOGE(videoplay, "File Read Error!");
                return res;
            }

            res = avi_init(pbuf, AVI_VIDEO_BUF_SIZE);
            if (res != 0)
            {
                ESP_LOGE(videoplay, "Avi Init Error!");
                return res;
            }

            if (g_avix.Width > 240 || g_avix.Height > 160) {
            ESP_LOGE(videoplay, "Video size too large: %ldx%ld", g_avix.Width, g_avix.Height);
            // 跳过该文件或进行缩放处理
            return 0xFF;  // 或直接跳过当前视频
            }

            video_info_show(&g_avix);                   /* 显示相关文件信息 */
            esptime_int_init(g_avix.SecPerFrame);       /* 初始化系统定时器 */
            offset = avi_search_id(pbuf, AVI_VIDEO_BUF_SIZE, "movi");   /* 寻找movi ID*/
            avi_get_streaminfo(pbuf + offset + 4);      /* 获取流ID信息 */
            f_lseek(favi, offset + 12);                 /* 跳过ID信息，读取地址偏移到流数据的起始处 */
            /* 初始化mjpeg解码器，并确定视频在spilcd上的显示位置*/
            res = mjpegdec_init((lcd_self.width - g_avix.Width) / 2, 110 + (lcd_self.height - 110 - g_avix.Height) / 2);
            if (res != 0) ESP_LOGE(videoplay, "mjpegdec_init fail!");
            Windows_Width = g_avix.Width;
            Windows_Height = g_avix.Height;

            if (g_avix.SampleRate)                      /* 有音频信息，则设置采样率和位宽 */
            {
                printf("g_avix.SampleRate:%ld\r\n", g_avix.SampleRate);
                es8311_update_fs(g_avix.SampleRate, I2S_DATA_BIT_WIDTH_16BIT);
                esp_codec_dev_set_out_vol(my_codec_dev, 50);    /*设置es8311音量*/
                xl9555_pin_write(SPK_EN_IO, 1);                 /*打开喇叭*/
            }

            while (1)
            {
                if (g_avix.StreamID == AVI_VIDS_FLAG)   /* 视频流*/
                {
                    pbuf = framebuf;
                    f_read(favi, pbuf, g_avix.StreamSize + 8, &br);     /* 读取整帧+下一帧流的ID信息*/
                    // ESP_LOGI(videoplay, "video frame size: %ld, read: %d, first bytes: %02X %02X", g_avix.StreamSize, br, pbuf[0], pbuf[1]);
                    // int64_t t0 = esp_timer_get_time();
                    res = mjpegdec_decode(pbuf, g_avix.StreamSize);     /* 解析并显示一帧图片 */
                    // int64_t t1 = esp_timer_get_time();
                    // ESP_LOGI("mjpegdec_decode", "total decode time:%lld", t1 - t0);

                    if (res != 0)
                    {
                        ESP_LOGE(videoplay, "Mjpeg decode error!");
                        break;
                    }

                    while (frameup == 0);                               /* 等待一帧的时间到达 */
                    frameup = 0;
                }
                else if (g_avix.StreamID == AVI_AUDS_FLAG)                                /* 音频流 */
                {
                    video_time_show(favi, &g_avix);                     /* 显示当前播放时间 */
                    f_read(favi, framebuf, g_avix.StreamSize + 8, &br);     /* 读取音频帧 */
                    // ESP_LOGI(videoplay, "audio frame size: %ld, read: %d", g_avix.StreamSize, br);
                    pbuf = framebuf;
                    // temp = es8311_write(framebuf, g_avix.StreamSize);          /* 发送给DAC*/
                    // if (temp != 0) ESP_LOGE(videoplay, "es8311_write fail: %d", temp);
                    xMessageBufferSend(audio_mbox, framebuf, g_avix.StreamSize, 0);
                }
                else ESP_LOGE(videoplay, "Stream ID Error!");

                key = xl9555_key_scan(0);
                if (key == KEY0_PRES || key == KEY1_PRES)               /* 播放下一个或上一个视频*/
                {
                    res = key;
                    break;
                }
                else if (key == KEY2_PRES || key == KEY3_PRES)          /* 视频快进或快退5秒*/
                {
                    i2s_trx_stop();
                    video_seek(favi, &g_avix, framebuf);
                    pbuf = framebuf;
                    i2s_trx_start();
                }

                if (avi_get_streaminfo(pbuf + g_avix.StreamSize) != 0)  /* 获取下一帧流的ID信息*/
                {
                    printf("Steam error or end of file");
                    res = KEY0_PRES;
                    break;
                }
            }

            lcd_wait_transfer_done();           /* 等待视频文件最后数据传输完毕 */
            esp_timer_stop(esp_tim_handle);
            mjpegdec_free();                    /* 释放内存 */
            f_close(favi);
        }
    }

    free(framebuf);
    free(favi);

    return res;
}

void video_play(void)
{
    uint8_t res;
    FF_DIR vdir;
    FILINFO* vfileinfo;
    uint8_t* pname;
    uint32_t* voffsettbl;
    uint16_t totavinum;
    uint16_t curindex;
    uint32_t temp;
    uint8_t key;

    while (f_opendir(&vdir, "0:/VIDEO") != FR_OK)           /* 检查文件夹是否存在 */
    {
        text_show_string(60, 190, 240, 16, "VIDEO文件夹不存在", 16, 0, RED, WHITE);
        vTaskDelay(200);
    }
    f_closedir(&vdir);

    totavinum = video_get_tnum("0:/VIDEO");                 /* 检查是否存在AVI视频文件 */
    while (totavinum == 0)
    {
        text_show_string(60, 190, 240, 16, "没有视频文件", 16, 0, RED, WHITE);
        vTaskDelay(200);
    }

    vfileinfo = (FILINFO*)malloc(sizeof(FILINFO));
    pname = malloc(255 * 2 + 1);
    voffsettbl = (uint32_t*)malloc(sizeof(uint32_t) * totavinum);

    while (vfileinfo == NULL || pname == NULL || voffsettbl == NULL)
    {
        text_show_string(60, 190, 240, 16, "内存分配失败", 16, 0, RED, WHITE);
        vTaskDelay(200);
    }

    res = (uint8_t)f_opendir(&vdir, "0:/VIDEO");
    if (res == 0)           /* 获取文件夹中文件信息对应的偏移字节，并放入偏移表中 */
    {
        curindex = 0;

        while (1)
        {
            temp = vdir.dptr;
            res = f_readdir(&vdir, vfileinfo);

            if (res != 0 || vfileinfo->fname[0] == 0) break;
            if (vfileinfo->fattrib & AM_DIR) continue;          /* AM: attribute mask */

            res = exfuns_file_type(vfileinfo->fname);
            ESP_LOGI(videoplay, "video file name: %s", vfileinfo->fname);
            if ((res & 0xF0) == 0x60)
            {
                voffsettbl[curindex] = temp;
                curindex++;
            }
        }
    }
    f_closedir(&vdir);

    curindex = 0;
    res = f_opendir(&vdir, "0:/VIDEO");

    while (res == 0)
    {
        dir_sdi(&vdir, voffsettbl[curindex]);
        res = f_readdir(&vdir, vfileinfo);

        if (res != 0 || vfileinfo->fname[0] == 0) break;

        strcpy((char*)pname, "0:/VIDEO/");
        strcat((char*)pname, (const char*)vfileinfo->fname);
        lcd_clear(WHITE);
        video_bmsg_show((uint8_t*)vfileinfo->fname, curindex + 1, totavinum);

        key = video_play_mjpeg(pname);      /* 解析显示视频 */

        if (key == KEY0_PRES)               /* 下一个视频 */
        {
            curindex++;
            if (curindex >= totavinum)
            {
                curindex = 0;
            }
        }
        else if (key == KEY1_PRES)          /* 上一个视频 */
        {
            if (curindex == 0)
            {
                curindex = totavinum - 1;
            }
            else curindex --;
        }
        else break;
    }

    f_closedir(&vdir);
    free(vfileinfo);
    free(pname);
    free(voffsettbl);
}

void video_task(void *arg) {
    while (1) {
        video_play();
    }
}

// 音频任务（核心0）
void audio_task(void *arg) {
    uint8_t data[1024];
    size_t len;
    size_t bytes_written;

    while (1) {
        len = xMessageBufferReceive(audio_mbox, data, sizeof(data), portMAX_DELAY);
        if (len > 0) {
            i2s_channel_write(tx_handle, data, len, &bytes_written, portMAX_DELAY);
        }
    }
}
