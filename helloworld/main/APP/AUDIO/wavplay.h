#pragma once /*wavplay.h WAV解码代码*/

#include "ff.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "my_es8311.h"
#include "xl9555.h"
#include "audioplay.h"

#define WAV_TX_BUFSIZE  1024 * 3    /*定义WAV TX数组大小*/

typedef struct {
    uint32_t ChunkID;       /*chunk id,这里固定为“RIFF”，即0X46464952*/
    uint32_t ChunkSize;     /*集合大小；文件总大小-8（不包括ID和Size大小）*/
    uint32_t Format;        /*格式；WAVE，即0X45564157*/
}ChunkRIFF;                 /*RIFF块*/

typedef struct{
    uint32_t ChunkID;               /*chunk id;这里固定为“fmt ”，即0X20746D66*/
    uint32_t ChunkSize;             /*子集合大小（不包括ID和Size）；这里为：16*/
    uint16_t AudioFormat;           /*音频格式；0x01表示线性PCM；0x11表示IMA ADPCM*/
    uint16_t NumOfChannels;         /*通道数量；1表示单声道；2表示双声道*/
    uint32_t SampleRate;            /*采样率；0x1F40，表示8Khz*/
    uint32_t ByteRate;              /*字节速率*/
    uint16_t BlockAlign;            /*块对齐（字节）*/
    uint16_t BitsPerSample;         /*单个采样数据大小（bit）；4位ADPCM，设置为4*/
    //  uint16_t ByteExtraData;     /*附加的数据字节；2个；线性PCM没有这个参数*/
}ChunkFMT;                          /*fmt块*/

/*在非PCM格式的文件中，一般会在Format结构后面加入一个Fact块*/
typedef struct{
    uint32_t ChunkID;               /*chunk id;这里固定为“fact”，即0X74636166*/
    uint32_t ChunkSize;             /*子集合大小（不包括ID和Size）；这里为4*/
    uint32_t NumOfSamples;          /*采样的数量*/
}ChunkFACT;                         /*fact块*/

typedef struct 
{
    uint32_t ChunkID;           /* chunk id;这里固定为"LIST",即0X5453494C; */
    uint32_t ChunkSize;         /* 子集合大小(不包括ID和Size); */
    //元信息……
} ChunkLIST;                    /* LIST块,可选的扩展信息块，存一些元数据，比如录音软件，版权说明或者备注之类的*/

typedef struct
{
    uint32_t ChunkID;           /* chunk id;这里固定为"data",即0X61746164 */
    uint32_t ChunkSize;         /* 子集合大小(不包括ID和Size),即音频数据大小 */
} ChunkDATA;                    /* data块 */

typedef struct
{
    ChunkRIFF riff;             /*riff块*/
    ChunkFMT fmt;               /*fmt块*/
//    ChunkFACT fact;           /*线性PCM中没有这个块*/
    ChunkDATA data;             /*data块*/
}_WaveHeader;                   /*WAV头*/

typedef struct{
    uint16_t audioformat;       /*音频格式；0x01表示线性PCM；0x11表示IMA ADPCM*/
    uint16_t nchannels;         /*通道数量；1表示单声道；2表示双声道*/
    uint16_t blockalign;        /*块对齐（字节）*/
    uint32_t datasize;          /*WAV音频数据大小*/

    uint32_t totsec;            /*整首歌时长，单位：秒*/
    uint32_t cursec;            /*当前播放时长*/

    uint32_t bitrate;           /*比特率（位速）*/
    uint32_t samplerate;        /*采样率*/
    uint16_t bps;               /*单次采样的位数，比如16bit，24bit，32bit*/

    uint32_t datastart;         /*数据帧开始的位置（在文件里的偏移字节）*/
}_wavctrl;                      /*wav播放控制结构体*/

/**
 * @brief 解析wav文件
 * @param fname:文件路径+文件名
 * @param wavx:输出的wav文件信息的结构体指针
 * @retval 0,成功解析
 *         1，打开文件失败
 *         2，非WAV文件
 *         3，DATA区域未找到
 */
uint8_t wav_decode_init(uint8_t* fname, _wavctrl* wavx);

/**
 * @brief 播放某个WAV文件
 * @param fname：文件路径+文件名
 * @retval: KEY0_PRES:下一首
 *          KEY1_PRES:上一首
 *          其他：失败
 */
uint8_t wav_play_song(uint8_t* fname);