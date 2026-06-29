#pragma once    /*my_jpeg_decode.h*/

#include "jpeg_decoder.h"

#define PIC_W   240
#define PIC_H   240

/**
 * @brief 解码JPEG图片数据
 * @param mjpegbuffer:jpeg格式的数据
 * @param size:jpeg格式数据的大小字节
 * @param outbuffer:需要输出格式的数据
 */
void decode_jpeg(uint8_t* mjpegbuffer, uint32_t size, uint8_t* outbuffer);