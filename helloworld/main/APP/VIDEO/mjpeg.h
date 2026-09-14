#pragma once /*mjpeg.h*/

#include <setjmp.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "jpeglib.h"
#include "lcd.h"
#include "jpeg_decoder.h"
#include "esptim_video.h"

struct my_error_mgr
{
    struct jpeg_error_mgr pub;      /* 公共字段 */
    jmp_buf setjpm_buffer;          /* 用于跳转返回 */
};
typedef struct my_error_mgr* my_error_ptr;

#define rgb565(r, g, b)    (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))


/**
 * @brief 解码一幅JPEG图片
 * @param buf: jpeg数据流数组
 * @param bsize：数组大小
 * @retval 0：成功；其他：失败
 */
uint8_t mjpegdec_decode(uint8_t* buf, uint32_t bsize);

/**
 * @brief mjpeg解码初始化
 * @param offx，offy：x，y方向的偏移
 * @retval: 0：成功；1：失败
 */
uint8_t mjpegdec_init(uint16_t offx, uint16_t offy);

void mjpegdec_free(void);