#pragma once /*audioplay.h 音乐播放代码*/

#include "wavplay.h"
#include "lcd.h"
#include "exfuns.h"
#include "text.h"

typedef struct {
    uint8_t* buf;       /*存储数据数组*/
    uint8_t* tbuf;      /*临时数组，仅在24Bit解码的时候需要用到*/
    FIL* file;          /*音频文件指针*/

    uint8_t status;     /*bit0：0，暂停播放，bit0：1，继续播放*/
                        /*bit1：0，结束播放，bit1：1，开启播放*/
}_audiodev;

extern _audiodev g_audiodev;

void audio_start(void);     /*开始播放音频*/
void audio_stop(void);      /*暂停播放音频*/

/**
 * @brief 在SPILCD上显示播放时长和总时长以及Kbps速率
 * @param totsec:总时长（秒）
 * @param cursec:当前播放时长（秒）
 * @param bitrate：传输的位率
 */
void audio_meg_show(uint32_t totsec, uint32_t cursec, uint32_t bitrate);
void audio_play(void);      /*播放音频*/