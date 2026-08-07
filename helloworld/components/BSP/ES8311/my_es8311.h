#pragma once    /*my_es8311.h */

#include "esp_codec_dev_defaults.h"
#include "esp_codec_dev.h"
#include "myi2s.h"
#include "es8311_codec.h"
#include "iic.h"

#define ES8311_SDPIN_REG09  0x09
#define ES8311_SDPOUT_REG0A 0x0A

// 与 es8311_codec.c 中完全相同的定义
typedef struct {
    audio_codec_if_t   base;
    es8311_codec_cfg_t cfg;
    bool               is_open;
    bool               enabled;
    float              hw_gain;
} audio_codec_es8311_t;


extern esp_codec_dev_handle_t my_codec_dev;


/**
 * @brief es8311初始化
 * @param self: i2c控制块
 * @return 0成功，其它失败
 */
uint8_t es8311_init(i2c_obj_t self);

/**
 * @brief 往es8311发送音频数据
 * @param data:要发送的音频数据
 * @param len:数据大小（字节）
 * @return 0成功，其它失败
 */
uint8_t es8311_write(uint8_t* data, int len);

/**
 * @brief es8311接收音频数据
 * @param data:要接收的音频数据缓冲区
 * @param len:缓冲区大小（字节）
 * @return 0成功，其它失败
 */
uint8_t es8311_read(uint8_t* data, int len);

/**
 * @brief 设置ES8311寄存器以及I2S的数据位宽和采样率
 * @param samplerate：要设置的采样率
 * @param bits：要设置的数据位宽
 * @return 0：成功；其他：错误
 */
esp_err_t es8311_update_fs(int samplerate, int bits);