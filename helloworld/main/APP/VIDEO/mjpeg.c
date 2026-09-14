#include "mjpeg.h"

const char* mjpeg = "mjpeg";
my_error_ptr jerr;
struct jpeg_decompress_struct* cinfo;
int Windows_Width = 0;
int Windows_Height = 0;
uint16_t imgoffx, imgoffy;              /*图像在x，y方向的偏移量*/

/**
 * @brief   错误退出
 * @param   cinfo:JPEG编码解码控制结构体
 * @retval  无
 */
METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message)(cinfo);           /* 显示错误信息 */
    longjmp(myerr->setjpm_buffer, 1);
}

/**
 * @brief   发出消息，是libjpeg用来输出“非致命消息”的回调，例如警告、调试信息等
 * @param   cinfo:JPEG编解码控制结构体
 * @param   msg_level:消息等级，负数表示错误或者警告，0或正数表示普通消息
 * @retval  无
 */
METHODDEF(void) my_emit_message(j_common_ptr cinfo, int msg_level)
{
    if (msg_level < 0)
    {
        ESP_LOGI(mjpeg, "emit message:%d\r\n", msg_level);
    }
}

METHODDEF(void) my_output_message(j_common_ptr cinfo)
{
    // ✅ 空实现，什么都不做，避免调用 format_message → strlen
}

uint16_t *lcd_buf[2] = {NULL, NULL};           /* 像素缓冲区,使用双缓冲区 */
static int current_buf = 0;

/* 这里使用libjpeg解码库，最终失败了，原因大概是该解码库的内存管理和ESP-IDF不匹配的原因*/
uint8_t mjpegdec_decode0(uint8_t* buf, uint32_t bsize)
{
    JSAMPARRAY buffer;

    // 调试用：打印 cinfo 地址，确认没有被覆盖
    ESP_LOGI(mjpeg, "cinfo=%p, jerr=%p, lcd_buf=%p", cinfo, jerr, lcd_buf);


    if (bsize == 0) return 1;

    // 检查 JPEG SOI 标记
    if (buf[0] != 0xFF || buf[1] != 0xD8) {
        ESP_LOGE(mjpeg, "Invalid JPEG data! First bytes: %02X %02X", buf[0], buf[1]);
        return 2;
    }
    // 检查 cinfo 和 jerr 是否有效
    if (cinfo == NULL || jerr == NULL || lcd_buf[0] == NULL) {
        ESP_LOGE(mjpeg, "JPEG decoder not initialized!");
        return 3;
    }

    int row_stride = 0;     /* 一行的像素的字节数 */
    int j = 0;              /* 记录当前解码的行数 */
    int lineR = 0;          /* 每一行R分量的起始位置 */

    cinfo->err = jpeg_std_error(&jerr->pub);
    jerr->pub.error_exit = my_error_exit;
    jerr->pub.output_message = my_output_message;  // ✅ 关键：覆盖 output_message
    jerr->pub.emit_message = my_output_message;    // ✅ 也覆盖这个
    cinfo->out_color_space = JCS_RGB;           /* JPEG Color Space这里是颜色设置为RGB格式*/

    if (setjmp(jerr->setjpm_buffer))    /* 错误处理 */
    {
        ESP_LOGE(mjpeg, "mjpegdec_decode:错误了进入setjmp");
        jpeg_abort_decompress(cinfo);
        return 2;
    }

    ESP_LOGI(mjpeg, "buf=%p, bsize=%lu, first4=%02X %02X %02X %02X", buf, bsize, buf[0], buf[1], buf[2], buf[3]);

    jpeg_create_decompress(cinfo);
    jpeg_mem_src(cinfo, buf, bsize);        /* 指定数据源 */
    jpeg_read_header(cinfo, TRUE);
    jpeg_start_decompress(cinfo);
    
    // 限制尺寸，防止越界
    int display_width = (cinfo->output_width > 240) ? 240 : cinfo->output_width;
    int display_height = (cinfo->output_height > 160) ? 160 : cinfo->output_height;

    /*一行字节数 = 一行像素 * 颜色分量（RGB也就是3个字节）*/
    row_stride = cinfo->output_width * cinfo->output_components;

    buffer = (*cinfo->mem->alloc_sarray)((j_common_ptr)cinfo, JPOOL_IMAGE, row_stride, 1);

    while (cinfo->output_scanline < display_height)
    {
        int k = 0;
        jpeg_read_scanlines(cinfo, buffer, 1);
        uint16_t tmp_color565;

        for (int i = 0; i < display_width; i++)
        {
            tmp_color565 = rgb565(buffer[0][k], buffer[0][k + 1], buffer[0][k + 2]);
            lcd_buf[lineR + i] = tmp_color565;

            k += 3;
        }
        
        j++;
        lineR = j * display_width;
    }

    // 如果图片实际高度小于 display_height，需要跳过剩余行
    while (cinfo->output_scanline < cinfo->output_height)
    {
        jpeg_read_scanlines(cinfo, buffer, 1);
    }

    lcd_draw_bitmap(imgoffx, imgoffy, imgoffx + cinfo->output_width, imgoffy + cinfo->output_height - 1, lcd_buf);

    ESP_LOGI(mjpeg, "Before finish: state=%d, output_scanline=%d, output_height=%d", cinfo->global_state, cinfo->output_scanline, cinfo->output_height);

    // ✅ 用 abort 代替 finish，避免内部调用 output_message
    jpeg_abort_decompress(cinfo);
    jpeg_destroy_decompress(cinfo);
    return 0;
}

uint8_t mjpegdec_init0(uint16_t offx, uint16_t offy)
{
    cinfo = (struct jpeg_decompress_struct*)calloc(1, sizeof(struct jpeg_decompress_struct));
    jerr = (my_error_ptr)calloc(1, sizeof(struct my_error_mgr));
    lcd_buf[0] = (uint16_t*)heap_caps_malloc(240 * 160 * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    ESP_LOGI(mjpeg, "lcd_buf=%p", lcd_buf);

    if (cinfo == NULL || jerr == NULL || lcd_buf[0] == NULL)
    {
        ESP_LOGE(mjpeg, "[E][mjpeg.cpp] mjpegdec_init(): malloc failed to apply for memory\r\n");
        mjpegdec_free();
        return 1;
    }

    /* 保存图像在x，y方向的偏移量 */
    imgoffx = offx;
    imgoffy = offy;

    return 0;
}

void mjpegdec_free0(void)
{
    if (cinfo != NULL)
    {
        jpeg_destroy_decompress(cinfo);  // ✅ 只在这里 destroy 一次
        free(cinfo);
        cinfo = NULL;
    }
    if (jerr != NULL)
    {
        free(jerr);
        jerr = NULL;
    }
    if (lcd_buf[0] != NULL)
    {
        heap_caps_free(lcd_buf[0]);
        lcd_buf[0] = NULL;
    }
}


/* 下面使用的esp_jpeg解码库，是成功的 */
uint8_t mjpegdec_decode(uint8_t* buf, uint32_t bsize)
{
    if (bsize < 4) return 1;
    if (buf[0] != 0xFF || buf[1] != 0xD8) return 2;         /* 检查每一帧mjpeg的起始标记是否正确 */
    if (lcd_buf[0] == NULL || lcd_buf[1] == NULL) return 3;

    esp_jpeg_image_cfg_t cfg = {
        .indata = buf,
        .indata_size = bsize,
        .outbuf = (uint8_t*)lcd_buf[current_buf],
        .outbuf_size = Windows_Width * Windows_Height * sizeof(uint16_t),
        .out_format = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale = JPEG_IMAGE_SCALE_0,         /*设置解码JPEG解码时的图片缩放比例，不缩放*/
    };
    esp_jpeg_image_output_t outimg;

    // int64_t t1 = esp_timer_get_time();
    esp_err_t ret = esp_jpeg_decode(&cfg, &outimg);
    if (ret != ESP_OK) {
        ESP_LOGE(mjpeg, "esp_jpeg_decode failed: %d", ret);
        return 2;
    }
    // int64_t t2 = esp_timer_get_time();

    // 发送到 LCD
    lcd_draw_bitmap(imgoffx, imgoffy, 
                    imgoffx + outimg.width - 1, 
                    imgoffy + outimg.height - 1, 
                    (uint16_t*)lcd_buf[current_buf]);
    current_buf ^= 1;   /* 切换缓冲区，下一帧解码到另一个缓冲区 */

    // int64_t t3 = esp_timer_get_time();
    // ESP_LOGI("time", "decode: %lld, lcd: %lld", t2-t1, t3-t2);

    return 0;
}

uint8_t mjpegdec_init(uint16_t offx, uint16_t offy)
{
    /* 分配lcd_buf到内部RAM */
    lcd_buf[0] = (uint16_t*)heap_caps_malloc(240*160*sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    lcd_buf[1] = (uint16_t*)heap_caps_malloc(240*160*sizeof(uint16_t), MALLOC_CAP_SPIRAM);

    if (lcd_buf[0] == NULL || lcd_buf[1] == NULL) 
    {
        ESP_LOGE(mjpeg, "alloc lcd_buf failed");
        mjpegdec_free();
        return 1;
    }

    current_buf = 0;
    imgoffx = offx;
    imgoffy = offy;
    return 0;
}

void mjpegdec_free(void)
{
    if (lcd_buf[0]) { heap_caps_free(lcd_buf[0]); lcd_buf[0] = NULL; }
    if (lcd_buf[1]) { heap_caps_free(lcd_buf[1]); lcd_buf[1] = NULL; }
}
