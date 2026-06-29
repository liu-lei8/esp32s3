#include "camera.h"

camera_fb_t* fb = NULL;

/*摄像头配置*/
static camera_config_t camera_cfg = {
    /*引脚配置*/
    .pin_pwdn = CAM_PIN_PWDN,
    .sccb_i2c_port = I2C_NUM_1,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    .pin_reset = CAM_PIN_RESET,
    .pin_pclk = CAM_PIN_PCLK,
    .pin_d0 = CAM_PIN_D0,
    .pin_d1 = CAM_PIN_D1,
    .pin_d2 = CAM_PIN_D2,
    .pin_d3 = CAM_PIN_D3,
    .pin_d4 = CAM_PIN_D4,
    .pin_d5 = CAM_PIN_D5,
    .pin_d6 = CAM_PIN_D6,
    .pin_d7 = CAM_PIN_D7,

    /*主时钟配置*/
    .xclk_freq_hz = 24000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    /*图像输出模式*/
    .pixel_format = PIXFORMAT_JPEG,

    /*图像输出大小*/
    .frame_size = FRAMESIZE_240X240,    

    /*0-63,对于OV系列摄像头传感器，数量越少代表质量越高 */
    .jpeg_quality = 12,
    
    /*当使用jpeg模式时，当fb_coount超过一个，则驱动程序在连续模式下工作*/
    .fb_count = 2,

    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

esp_err_t camera_init(void)
{
    if(CAM_PIN_PWDN == GPIO_NUM_NC)
    {
        CAM_PWDN(0);        /*掉电使能*/
    }

    if(CAM_PIN_RESET == GPIO_NUM_NC)
    {
        CAM_RST(0);
        vTaskDelay(pdMS_TO_TICKS(20));
        CAM_RST(1);
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    /*摄像头初始化*/
    esp_err_t err = esp_camera_init(&camera_cfg);

    if (err != ESP_OK)
    {
        ESP_LOGE("camera", "Camera Init Fail!");
        return err;
    }

    sensor_t* s = esp_camera_sensor_get();

    /*如果摄像头是OV3660或者OV5640，则需要以下配置*/
    if (s->id.PID == OV3660_PID)
    {
        s->set_vflip(s, 1);          /*向后翻转*/
        s->set_brightness(s, 1);     /*亮度提高*/
        s->set_saturation(s , -2);   /*饱和度降低*/
    }
    else if (s->id.PID == OV5640_PID)
    {
        s->set_vflip(s, 1);         /*向后翻转*/
    }

    return err;
}

void camera_show(uint16_t x0, uint16_t y0, uint8_t* data)
{
    spi_transaction_t trans;
    uint8_t* line_buf;

    memset(&trans, 0, sizeof(trans));       /*将发送和接收缓冲区清空*/

    if (data)
    {
        line_buf = data;
    }
    else
    {
        return;
    }

    lcd_address_set(x0, LCD_W - 1, y0, 240 - 1);      /*240 * 240分辨率*/
    LCD_WR(1);      /*准备写数据*/


    /*逐行处理，对于每一行y*/
    for (uint8_t y = 0; y < 240; y++)
    {
        line_buf = data + y * 240 * 2;   /*fb->buf的第0个字节是16位颜色的高字节*/

        trans = (spi_transaction_t){.tx_buffer = line_buf, .length = LCD_W * 2 * 8};

        spi_device_polling_transmit(my_lcd_handle, &trans); /*发送一行数据*/
    }
}
