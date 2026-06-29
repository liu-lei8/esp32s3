#include "my_jpeg_decode.h"

void decode_jpeg(uint8_t* mjpegbuffer, uint32_t size, uint8_t* outbuffer)
{
    esp_jpeg_image_cfg_t jpeg_cfg = {
        .indata = mjpegbuffer,
        .indata_size = size,
        .outbuf = outbuffer,
        .outbuf_size = PIC_W * PIC_H * 2,
        .out_format = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale = JPEG_IMAGE_SCALE_0,        /*设置解码JPEG解码时的图片缩放比例*/
        .flags.swap_color_bytes = 1,            /*交换16位颜色值的高低字节，先发送高字节*/
    };
    esp_jpeg_image_output_t outimg;
    esp_jpeg_decode(&jpeg_cfg, &outimg);
}