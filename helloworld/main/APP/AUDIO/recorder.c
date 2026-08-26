#include "recorder.h"

uint8_t g_rec_sta = 0;  /**录音状态
                         * [7]:0,没有开启录音，1开启录音
                         * [6:1]:保留
                         * [0]:0,正在录音，1，暂停录音 */

SemaphoreHandle_t i2s_mutex;
static QueueSetHandle_t xQueueSet;      /*定义队列集*/
SemaphoreHandle_t key0_xSemaphore;      /*定义二值信号量*/
SemaphoreHandle_t key1_xSemaphore;      /*定义二值信号量*/
SemaphoreHandle_t key3_xSemaphore;      /*定义二值信号量*/

uint32_t recsec = 0;                    /*录音时间*/
_WaveHeader* wavhead = NULL;            /*wav头部指针*/
/*双缓冲区机制*/
uint8_t* current_buf;
uint8_t* next_buf;
uint32_t rec_read_total_bytes = 0;      /*实际读到录音的总数据*/
uint32_t wav_write_total_bytes = 0;     /*实际写入WAV音频的总数据*/

static void recorder_msg_show(uint32_t tsec, uint32_t bps)  /*显示录音时间和码率到SPILCD*/
{
    lcd_show_string(30, 210, "TIME:", RED, WHITE, 16, 1);           /*显示录音时间*/
    lcd_show_num(30 + 40, 210, tsec / 60, 2, RED, WHITE, 16, 0);    /*显示分钟*/
    lcd_show_char(30 + 56, 210, ':', RED, WHITE, 16, 1);
    lcd_show_num(30 + 64, 210, tsec % 60, 2, RED, WHITE, 16, 0);    /*显示秒*/

    lcd_show_string(140, 210, "KBPS:", RED, WHITE, 16, 1);          /*显示码率*/
    lcd_show_num(140 + 40, 210, bps / 1000, 4, RED, WHITE, 16, 0);  /*码率显示*/
}

static void recorder_remindmsg_show(uint8_t mode)       /*显示提示信息到SPILCD，0录音提示，1播放提示*/
{
    lcd_fill(30, 120, lcd_self.width, lcd_self.height, WHITE);

    if (mode == 0)      /*录音模式*/
    {
        lcd_show_string(30, 120, "KEY0:REC/PAUSE", BLUE, WHITE, 16, 1);
        lcd_show_string(30, 140, "KEY1:STOP&SAVE", BLUE, WHITE, 16, 1);
        lcd_show_string(30, 160, "KEY3:PLAY", BLUE, WHITE, 16, 1);
    }
    else                /*播放模式*/
    {
        lcd_show_string(30, 120, "KEY0:STOP PLAY", BLUE, WHITE, 16, 1);
        lcd_show_string(30, 140, "KEY3:PLAY/PAUSE", BLUE, WHITE, 16, 1);
    }
}

static void recorder_enter_rec_mode(void)               /*进入录音模式*/
{
    // i2s_trx_stop();       /*先停止可能存在的播放和录音*/

    es8311_update_fs(REC_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT);    /*根据录音需求设置采样率和位宽*/

    esp_codec_dev_set_in_gain(my_codec_dev, 30);        /*开启ADC增益*/

    xl9555_pin_write(SPK_EN_IO, 0);                     /*关闭喇叭*/
    esp_codec_dev_set_out_mute(my_codec_dev, true);     /*设置喇叭静音*/

    // i2s_trx_start();                                    /*开启I2S*/
    recorder_remindmsg_show(0);                         /*显示录音信息*/
}

static void recorder_enter_play_mode(void)              /*进入播放模式*/
{
    // i2s_trx_stop();                                     /*停止录音*/

    esp_codec_dev_set_out_mute(my_codec_dev, false);    /*关闭静音*/
    esp_codec_dev_set_in_gain(my_codec_dev, 0);         /*关闭ADC增益*/

    recorder_remindmsg_show(1);
}

static void recorder_wav_init(_WaveHeader* wavhead)     /*WAV头初始化*/
{
    wavhead->riff.ChunkID = 0X46464952;             /*RIFF*/
    wavhead->riff.ChunkSize = 0;                    /*未确定，需要最后计算*/
    wavhead->riff.Format = 0X45564157;              /*WAVE*/

    wavhead->fmt.ChunkID = 0X20746D66;              /*"fmt "*/
    wavhead->fmt.ChunkSize = 16;                    /*PCM格式块大小为16字节*/
    wavhead->fmt.AudioFormat = 0x01;                /*0x01，表示线性PCM; 0x11表示IMA PCM*/
    wavhead->fmt.NumOfChannels = 2;                 /*双声道*/
    wavhead->fmt.SampleRate = REC_SAMPLE_RATE;      /*采样速率为16000Hz*/
    wavhead->fmt.BitsPerSample = 16;                /*16位的PCM*/
    wavhead->fmt.BlockAlign = wavhead->fmt.NumOfChannels * (wavhead->fmt.BitsPerSample / 8);    /*块对齐=通道数*每个样本的字节数*/
    wavhead->fmt.ByteRate = wavhead->fmt.SampleRate * wavhead->fmt.BlockAlign;  /*字节速率=采样率*快对齐*/

    wavhead->data.ChunkID = 0X61746164;             /*"data"*/
    wavhead->data.ChunkSize = 0;                    /*数据大小，录音后再算*/

}

static void recorder_new_pathname(uint8_t* pname)       /*从SD卡获取录音路径+新的文件名到pname*/
{
    uint8_t res;
    uint16_t index = 0;
    FIL* ftemp;
    
    ftemp = (FIL*)malloc(sizeof(FIL));
    if (ftemp == NULL)
    {
        return;
    }

    while (index < 0xFFFF)
    {
        sprintf((char*)pname, "0:RECORDER/REC%05d.wav", index);
        res = f_open(ftemp, (const TCHAR*)pname, FA_READ);

        if (res == FR_NO_FILE)
        {
            break;                  /*该文件名不存在，这正是我们需要的文件名*/
        }

        index++;
    }

    free(ftemp);
}

static void recorder_task(void* arg)                    /*WAV录音任务*/
{
    QueueSetMemberHandle_t activate_member = NULL;
    FF_DIR recdir;
    uint8_t* pname;
    uint8_t rval = 0;                   /*返回状态*/
    FSIZE_t rec_file_read_pos = 0;
    uint8_t file_write_res = 0;
    FIL* f_rec = NULL;                  /*录音文件*/
    UINT bw;                            /*实际写的字节数*/


    pname = malloc(255);
    f_rec = (FIL*)malloc(sizeof(FIL));
    wavhead = (_WaveHeader*)malloc(sizeof(_WaveHeader));
    current_buf = malloc(REC_RX_BUF_SIZE);
    next_buf = malloc(REC_RX_BUF_SIZE);

    if (!pname || !f_rec || !wavhead || !current_buf || !next_buf)
    {
        goto exit;
    }

    /*打开文件夹，若没有则自动创建*/
    while (f_opendir(&recdir, "0:/RECORDER"))
    {
        f_mkdir("0:/RECORDER");
    }

    xTaskNotifyGive(arg);
    /*创建互斥锁，防止数据存储干扰*/
    i2s_mutex = xSemaphoreCreateRecursiveMutex();
    xSemaphoreGiveRecursive(i2s_mutex);

    recorder_enter_rec_mode();      /*进入录音模式*/
    pname[0] = 0;

    while (rval == 0)
    {
        activate_member = xQueueSelectFromSet(xQueueSet, 10);

        if (activate_member == key0_xSemaphore)          /*录制/暂停*/
        {
            xSemaphoreTake(activate_member, portMAX_DELAY);

            if ((g_rec_sta & 0x01) && rec_file_read_pos != 0 && file_write_res == 1)    /*暂停录音*/
            {
                g_rec_sta &= ~0x01;      /*恢复录音*/
            }
            else if ((g_rec_sta & 0x80) && file_write_res == 1)                         /*正在录音*/
            {
                g_rec_sta |= 0x01;      /*暂停录音*/
                rec_file_read_pos = f_tell(f_rec);      /*记录当前文件写到哪个位置*/
            }
            else                                                                        /*未开始录音*/
            {
                recsec = 0;
                recorder_new_pathname(pname);       /*获取路径+新文件名*/
                text_show_string(30, 190, lcd_self.width, 16, "录制：", 16, 1, RED, WHITE); /*显示文件名*/
                text_show_string(30 + 16 * 3, 190, lcd_self.width, 16, (char*)pname + 11, 16, 1, RED, WHITE);
                recorder_wav_init(wavhead);         /*初始化WAV头*/
                es8311_update_fs(wavhead->fmt.SampleRate, wavhead->fmt.BitsPerSample);  /*内部自动启动I2S*/
                rval = f_open(f_rec, (const TCHAR*)pname, FA_WRITE | FA_CREATE_NEW);
                if (rval != FR_OK)      /*打开文件失败*/
                {
                    g_rec_sta = 0;      /*停止录制*/
                    rval = 0xFE;        /*提示SD有问题*/
                }
                else
                {
                    f_lseek(f_rec, sizeof(_WaveHeader));    /*偏移到录音数据的存储地址*/
                    g_rec_sta |= 0x80;                      /*打开录制*/
                    recorder_msg_show(0, 0);                /*开始显示录制时间*/
                }
            }
        }
        else if (activate_member == key1_xSemaphore)     /*停止&保存*/
        {
            xSemaphoreTake(activate_member, portMAX_DELAY);
            /*在录音状态，且上一次写入成功，并且读到和写入相等*/
            if (g_rec_sta & 0x80 && file_write_res == 1 && rec_read_total_bytes == wav_write_total_bytes)
            {
                xSemaphoreTakeRecursive(i2s_mutex, portMAX_DELAY);
                g_rec_sta = 0;      /*停止录制*/
                wavhead->riff.ChunkSize = wav_write_total_bytes + 36;
                wavhead->data.ChunkSize = wav_write_total_bytes;
                f_lseek(f_rec, 0);  /*起始地址写入文件头*/
                f_write(f_rec, wavhead, sizeof(_WaveHeader), &bw);      /*写入文件头*/
                f_close(f_rec);
                xSemaphoreGiveRecursive(i2s_mutex);
            }
            rec_read_total_bytes = 0;
            wav_write_total_bytes = 0;
            g_rec_sta = 0;
            recsec = 0;
            lcd_fill(30, 190, lcd_self.width, lcd_self.height, WHITE);
        }
        else if (activate_member == key3_xSemaphore)     /*播放录音*/
        {
            xSemaphoreTake(activate_member, portMAX_DELAY);
            if (g_rec_sta == 0 && file_write_res == 1)
            {
                recorder_enter_play_mode();     /*进入播放模式*/
                text_show_string(30, 190, lcd_self.width, 16, "播放：", 16, 1, RED, WHITE);
                text_show_string(30 + 16 * 3, 190, lcd_self.width, 16, (char*)pname + 11, 16, 1, RED, WHITE);
                wav_play_song(pname);
                recorder_enter_rec_mode();      /*回到录制模式*/
                g_rec_sta = 0;                  /*清除状态*/
            }
        }

        if ((g_rec_sta & 0x80) && ((g_rec_sta & 0x01) != 1))  /*开始录制音频并写入WAV文件*/
        {
            xSemaphoreTakeRecursive(i2s_mutex, portMAX_DELAY);
            file_write_res = 0;

            rval = esp_codec_dev_read(my_codec_dev, current_buf, REC_RX_BUF_SIZE);
            if (rval == ESP_CODEC_DEV_OK)
            {
                rec_read_total_bytes += REC_RX_BUF_SIZE;

                if (rec_file_read_pos != 0)
                {
                    f_lseek(f_rec, rec_file_read_pos);
                    rec_file_read_pos = 0;
                }
                rval = f_write(f_rec, current_buf, REC_RX_BUF_SIZE, &bw);
                if (rval == FR_OK)
                {
                    wav_write_total_bytes += bw;
                    uint8_t* temp = current_buf;        /*切换缓冲区*/
                    current_buf = next_buf;
                    next_buf = temp;
                    file_write_res = 1;
                }
                else ESP_LOGE("recorder_task", "write fail: %d", rval);
            }
            xSemaphoreGiveRecursive(i2s_mutex);
        }
    }

exit:
    /*清理资源*/
    free(pname);
    free(f_rec);
    free(wavhead);
    free(current_buf);
    free(next_buf);
    /*删除任务*/
    vTaskDelete(NULL);
}

void wav_recorder(void)                                 /*录音主程序*/
{
    uint8_t key = 0;
    uint8_t timecnt = 0;
    uint32_t current_rectime = 0;

    /*创建录音线程*/
    BaseType_t task_created = xTaskCreatePinnedToCore(recorder_task, "recorder_task", 4096, xTaskGetCurrentTaskHandle(), 10, NULL, 0);

    assert(task_created == pdTRUE);

    /*等待录音任务线程通知才往下执行*/
    ulTaskNotifyTake(false, portMAX_DELAY);
    /*创建队列集*/
    xQueueSet = xQueueCreateSet(3);
    /*创建二值信号量*/
    key0_xSemaphore = xSemaphoreCreateBinary();
    key1_xSemaphore = xSemaphoreCreateBinary();
    key3_xSemaphore = xSemaphoreCreateBinary();
    /*把信号量加入到队列集*/
    xQueueAddToSet(key0_xSemaphore, xQueueSet);
    xQueueAddToSet(key1_xSemaphore, xQueueSet);
    xQueueAddToSet(key3_xSemaphore, xQueueSet);

    while (1)
    {
        key = xl9555_key_scan(0);
        switch (key)
        {
        case KEY0_PRES:
            xSemaphoreGive(key0_xSemaphore);
            break;
        case KEY1_PRES:
            xSemaphoreGive(key1_xSemaphore);
            break;
        case KEY3_PRES:
            xSemaphoreGive(key3_xSemaphore);
            break;
        default:
            break;
        }

        current_rectime = wav_write_total_bytes / (REC_SAMPLE_RATE * 4);

        if (recsec != current_rectime)
        {
            recsec = current_rectime;
            recorder_msg_show(recsec, wavhead->fmt.ByteRate * 8);
        }

        timecnt++;
        if (timecnt % 20 == 0)
        {
            LED_TOGGLE();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

}
