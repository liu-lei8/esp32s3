#include "bsp/camera.h"
#include "esp_log.h"

static const char *TAG = "OV5640";

static camera_config_t camera_cfg = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 24000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565,
    .frame_size = FRAMESIZE_240X240,    
    .jpeg_quality = 12,
    .fb_count = 2,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    .sccb_i2c_port = I2C_NUM_1, // 摄像头专用 I2C 端口
};

esp_err_t camera_init(void)
{
    if(CAM_PIN_PWDN == GPIO_NUM_NC) {
        CAM_PWDN(0);
    }
    if(CAM_PIN_RESET == GPIO_NUM_NC) {
        CAM_RST(0);
        vTaskDelay(pdMS_TO_TICKS(20));
        CAM_RST(1);
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    esp_err_t err = esp_camera_init(&camera_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Fail!");
        return err;
    }

    sensor_t* s = esp_camera_sensor_get();
    if (s->id.PID == OV5640_PID) {
        s->set_vflip(s, 1);         /*向后翻转*/
        s->set_hmirror(s, 1); // 如果画面左右反了，可以加上这句
    }
    ESP_LOGI(TAG, "Camera initialized OK");
    return err;
}