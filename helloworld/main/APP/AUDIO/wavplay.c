#include "wavplay.h"

/************************************************************************************ */
/*FreeRTOS配置*/

/* MUSIC任务配置
 * 包括：任务句柄、任务优先级、堆栈大小、创建任务
 */
#define MUSIC_PRIO      4           /*任务优先级*/
#define MUSIC_STK_SIZE  5 * 1024    /*任务堆栈大小*/
TaskHandle_t music_task_handler;    /*任务句柄*/
void music(void* pvParameters);     /*任务函数*/

static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;

/************************************************************************************ */

_wavctrl wavctrl;                           /*WAV音频解码参数结构体*/
UINT bytes_written;                         /*记录每次实际写入的音频数据*/
volatile long long int i2s_table_size = 0;  /*累计每次发送音频的总大小*/
FSIZE_t file_read_pos = 0;                  /*记录当前WAV文件读取位置*/
esp_err_t i2s_play_next_prev = ESP_FAIL;    /*下一首或者上一首的标志位*/
esp_err_t i2s_play_end = ESP_FAIL;          /*播放结束的标志位*/

uint8_t wav_decode_init(uint8_t* fname, _wavctrl* wavx)
{
    FIL* ftemp;
    uint8_t* buf;
    uint8_t res = 0;
    uint32_t br = 0;

    ChunkRIFF* riff;
    ChunkFMT* fmt;
    ChunkFACT* fact;
    ChunkDATA* data;

    ftemp = (FIL*)malloc(sizeof(FIL));
    buf = malloc(512);

    if (ftemp && buf)
    {
        res = f_open(ftemp, (const TCHAR*)fname, FA_READ);      /*打开文件*/
        if (res == FR_OK)
        {
            f_read(ftemp, buf, 512, (UINT)&br);         /*读取512个字节到缓冲区*/
            riff = (ChunkRIFF*)buf;

            if (riff->Format == 0X45564157)                     /*是WAV文件*/
            {
                fmt = (ChunkFMT*)(buf + 12);
                fact = (ChunkFACT*)(buf + 12 + 8 + fmt->ChunkSize);

                if (fact->ChunkID == 0X74636166 || fact->ChunkID == 0X5453494C) /*连带LIST块一起判断了*/
                {
                    wavx->datastart = 12 + 8 + fmt->ChunkSize + 8 + fact->ChunkSize;
                }
                else
                {
                    wavx->datastart = 12 + 8 + fmt->ChunkSize;
                }

                data = (ChunkDATA*)(buf + wavx->datastart);

                if (data->ChunkID == 0X61746164)
                {
                    wavx->audioformat = fmt->AudioFormat;
                    wavx->nchannels = fmt->NumOfChannels;
                    wavx->samplerate = fmt->SampleRate;
                    wavx->bitrate = fmt->ByteRate * 8;
                    wavx->blockalign = fmt->BlockAlign;
                    wavx->bps = fmt->BitsPerSample;

                    wavx->datasize = data->ChunkSize;
                    wavx->datastart = wavx->datastart + 8;      /*不包括ID和Size*/

                    printf("wavx->audioformat:%d\r\n", wavx->audioformat);
                    printf("wavx->nchannels:%d\r\n", wavx->nchannels);
                    printf("wavx->samplerate:%ld\r\n", wavx->samplerate);
                    printf("wavx->bitrate:%ld\r\n", wavx->bitrate);
                    printf("wavx->blockalign:%d\r\n", wavx->blockalign);
                    printf("wavx->bps:%d\r\n",wavx->bps);
                    printf("wavx->datasize:%ld\r\n", wavx->datasize);
                    printf("wavx->datastart:%ld\r\n",wavx->datastart);
                }
                else
                {
                    res = 3;    /*DATA区域未找到*/
                }
            }
            else
            {
                res = 2;    /*非WAV文件*/
            }
        }
        else
        {
            res = 1;        /*打开文件失败*/
        }
    }

    f_close(ftemp);
    free(ftemp);
    free(buf);

    return res;
}

/**
 * @brief 获取当前音频播放的时间和总时间
 * @param fx:文件指针
 * @param wavx：要输出的wav控制块
 */
void wav_get_curtime(FIL* fx, _wavctrl* wavx)
{
    long long fpos = 0;

    wavx->totsec = wavx->datasize / (wavx->bitrate / 8);    /*歌曲总长度（单位：秒）*/
    fpos = fx->fptr - wavx->datastart;                      /*获取已经播放的长度字节*/
    wavx->cursec = (fpos * wavx->totsec) / wavx->datasize;    /*已经播放的时长秒*/
}

/**
 * @brief music任务
 * @param pvParameters:传入参数（未用到
 */
void music(void* pvParameters)
{
    uint16_t samples;
    uint8_t* src;
    uint8_t* dst;

    esp_codec_dev_set_out_vol(my_codec_dev, 50);    /*设置es8311音量*/
    xl9555_pin_write(SPK_EN_IO, 1);                 /*打开喇叭*/
    esp_codec_dev_write(my_codec_dev, g_audiodev.buf, WAV_TX_BUFSIZE); /*传输一段无声音的数据*/

    while (1)
    {
        if ((g_audiodev.status & 0x0F) == 0x03)     /*打开了音频*/
        {
            for (uint16_t readTimes = 0; readTimes < (wavctrl.datasize / WAV_TX_BUFSIZE) + 1; readTimes++)
            {
                if ((g_audiodev.status & 0x0F) == 0x00)         /*暂停播放*/
                {
                    file_read_pos = f_tell(g_audiodev.file);    /*记录暂停位置*/

                    while (1)                                   /*死等直到重新打开*/
                    {
                        if ((g_audiodev.status & 0x0F) == 0x03)
                        {
                            break;
                        }
                        vTaskDelay(pdMS_TO_TICKS(5));
                    }

                    f_lseek(g_audiodev.file, file_read_pos);    /*跳到之前停止的位置*/
                }

                if (i2s_table_size >= wavctrl.datasize || i2s_play_next_prev == ESP_OK)
                {
                    // audio_stop();                   /*先停止播放*/
                    i2s_table_size = 0;             /*总大小清零*/
                    i2s_play_end = ESP_OK;          /*一首歌结束*/
                    music_task_handler = NULL;
                    vTaskDelete(NULL);              /*删除当前任务*/
                    vTaskDelay(pdMS_TO_TICKS(5));   /*适当延迟，为删除任务*/
                    break;                          /*防止延迟5ms未能删除任务，强制退出*/
                }

                f_read(g_audiodev.file, g_audiodev.buf, WAV_TX_BUFSIZE, &bytes_written);
                if (bytes_written > 0)
                {
                    // if (wavctrl.bps == 24)                   /*24位宽数据时*/
                    // {
                    //     samples = bytes_written / 3;        /*获取采样数*/
                    //     for (uint16_t i = 0; i < samples; i++)  /*24位宽数据转化为32位对齐*/
                    //     {
                    //         g_audiodev.tbuf[i * 4] = g_audiodev.buf[i * 3];
                    //         g_audiodev.tbuf[i * 4 + 1] = g_audiodev.buf[i * 3 + 1];
                    //         g_audiodev.tbuf[i * 4 + 2] = g_audiodev.buf[i * 3 + 2];
                    //         g_audiodev.tbuf[i * 4 + 3] = 0;
                    //     }
                    //     esp_codec_dev_write(my_codec_dev, g_audiodev.tbuf, samples * 4);
                    // }
                    // else                                    /*传输16位宽数据*/
                    // {
                    //     esp_codec_dev_write(my_codec_dev, g_audiodev.buf, bytes_written);
                    // }
                    esp_codec_dev_write(my_codec_dev, g_audiodev.buf, bytes_written);
                    i2s_table_size += bytes_written;
                }
                else break;     /*没有数据可读，直接结束掉*/
                
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    music_task_handler = NULL;
    vTaskDelete(NULL);
}

uint8_t wav_play_song(uint8_t* fname)
{
    uint8_t key = 0;
    uint8_t res = 0;

    i2s_play_end = ESP_FAIL;
    i2s_play_next_prev = ESP_FAIL;
    g_audiodev.file = (FIL*)heap_caps_malloc(sizeof(FIL), MALLOC_CAP_DMA);
    g_audiodev.buf = heap_caps_malloc(WAV_TX_BUFSIZE, MALLOC_CAP_DMA);
    g_audiodev.tbuf = heap_caps_malloc(WAV_TX_BUFSIZE / 3 * 4, MALLOC_CAP_DMA); /*供24位宽数据传输用，要32位对齐*/

    if (g_audiodev.file && g_audiodev.tbuf && g_audiodev.buf)
    {
        memset(g_audiodev.file, 0, sizeof(FIL));
        memset(g_audiodev.tbuf, 0, WAV_TX_BUFSIZE);
        memset(&wavctrl, 0, sizeof(_wavctrl));
        res = wav_decode_init(fname, &wavctrl);

        if (res == ESP_OK)              /*解码成功*/
        {
            // if (wavctrl.bps == 16)
            // {
            //     i2s_set_samplerate_bits_sample(wavctrl.samplerate, I2S_DATA_BIT_WIDTH_16BIT);
            // }
            // else if (wavctrl.bps == 24)
            // {
            //     i2s_set_samplerate_bits_sample(wavctrl.samplerate, I2S_DATA_BIT_WIDTH_24BIT);
            // }
            es8311_update_fs(wavctrl.samplerate, wavctrl.bps);              /*内部会自动停止发送I2S音频数据*/

            res = f_open(g_audiodev.file, (const TCHAR*)fname, FA_READ);    /*打开文件*/
            f_lseek(g_audiodev.file, wavctrl.datastart);                    /*定位到文件音频数据位置*/

            if (res == ESP_OK)
            {
                g_audiodev.status = 0x03;
                // audio_start();      /*开启I2S*/
                if (music_task_handler == NULL && res == ESP_OK)            /*创建任务*/
                {
                    taskENTER_CRITICAL(&my_spinlock);                       /*避免双核竞争运行，只用一个核*/
                    xTaskCreate(music, "music", MUSIC_STK_SIZE, NULL, MUSIC_PRIO, &music_task_handler);
                    taskEXIT_CRITICAL(&my_spinlock);
                }

                while (1)
                {
                    if (i2s_play_end == ESP_OK)     /*一首歌放完就接着下一首*/
                    {
                        res = KEY0_PRES;            /*自动切下一首歌*/
                        break;
                    }

                    key = xl9555_key_scan(0);
                    switch(key)
                    {
                        case KEY0_PRES:
                        case KEY1_PRES:
                            i2s_play_next_prev = ESP_OK;
                            res = key;                          /*用户按键切歌*/
                            break;
                        case KEY3_PRES:     /*启动/暂停*/
                            if ((g_audiodev.status & 0x0F) == 0x00)
                            {
                                audio_start();
                            }
                            else
                            {
                                audio_stop();
                            }
                            break;
                        default: break;
                    }

                    if (res == KEY0_PRES || res == KEY1_PRES) break;

                    if ((g_audiodev.status & 0x0F) == 0x03)        /*暂停时不用更新显示时间*/
                    {
                        wav_get_curtime(g_audiodev.file, &wavctrl);     /*获取当前播放时间*/
                        audio_meg_show(wavctrl.totsec, wavctrl.cursec, wavctrl.bitrate);    /*显示播放时间信息*/
                    }

                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
            else
            {
                res = 0xFF; /*文件打开失败*/
            }
        }
        else
        {
            res = 0xFF;     /*解码失败*/
        }
    }
    else
    {
        res = 0xFF;         /*内存分配失败*/
    }

    while (music_task_handler != NULL)
    {
        vTaskDelay(5);
    }
    heap_caps_free(g_audiodev.file);
    heap_caps_free(g_audiodev.buf);
    heap_caps_free(g_audiodev.tbuf);
    g_audiodev.file = NULL;
    g_audiodev.buf = NULL;
    g_audiodev.tbuf = NULL;

    return res;
}

