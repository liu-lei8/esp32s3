#include "qr_code.hpp"
#include "bsp/esp-bsp.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "quirc.h"

extern "C" {
    #include "bsp/camera.h"
    #include "bsp/display.h"
}

// 声明之前实现的字符串打印函数
extern "C" void frame_printf(uint16_t *frame, int width, int height, int x, int y, uint16_t color, uint8_t sizey, const char *format, ...);

static const char *TAG = "QR_Code";

// 将 RGB565 图像转换为 8 位灰度图（修复字节序版）
static void rgb565_to_grayscale(const uint8_t *rgb565_buf, uint8_t *gray_buf, int width, int height)
{
    int total = width * height;
    for (int i = 0; i < total; i++) {
        uint8_t high_byte = rgb565_buf[i * 2];
        uint8_t low_byte  = rgb565_buf[i * 2 + 1];
        uint16_t color = (high_byte << 8) | low_byte;
        
        uint8_t r = (color >> 11) & 0x1F;
        uint8_t g = (color >> 5) & 0x3F;
        uint8_t b = color & 0x1F;
        
        uint8_t r8 = (r << 3) | (r >> 2);
        uint8_t g8 = (g << 2) | (g >> 4);
        uint8_t b8 = (b << 3) | (b >> 2);
        
        gray_buf[i] = (uint8_t)((r8 * 76 + g8 * 150 + b8 * 29) >> 8);
    }
}

// 绘制二维码框和解码内容
static void draw_qr_result(uint16_t *lcd_buf, int width, int height, struct quirc *qr, int count)
{
    uint16_t box_color = 0xE007; // 绿色

    for (int i = 0; i < count; i++) {
         // 【关键修改】加上 static，让它们分配到静态数据区，彻底释放栈空间，这两个结构体变量合计可能占用十几KB
        static struct quirc_code code; 
        static struct quirc_data data;
        
        quirc_extract(qr, i, &code);
        quirc_decode_error_t err = quirc_decode(&code, &data);

        // 计算外接矩形
        int min_x = width, max_x = 0, min_y = height, max_y = 0;
        for (int j = 0; j < 4; j++) {
            if (code.corners[j].x < min_x) min_x = code.corners[j].x;
            if (code.corners[j].x > max_x) max_x = code.corners[j].x;
            if (code.corners[j].y < min_y) min_y = code.corners[j].y;
            if (code.corners[j].y > max_y) max_y = code.corners[j].y;
        }

        if (min_x < 0) min_x = 0;
        if (min_y < 0) min_y = 0;
        if (max_x >= width) max_x = width - 1;
        if (max_y >= height) max_y = height - 1;

        // 画框
        for (int x = min_x; x <= max_x; x++) {
            lcd_buf[min_y * width + x] = box_color;
            lcd_buf[max_y * width + x] = box_color;
        }
        for (int y = min_y; y <= max_y; y++) {
            lcd_buf[y * width + min_x] = box_color;
            lcd_buf[y * width + max_x] = box_color;
        }

        if (err != QUIRC_SUCCESS) {
            // 【调试打印】把具体的错误码打印到串口
            ESP_LOGE(TAG, "Decode failed with error code: %d", err);
            
            // 【尝试镜像翻转】
            quirc_flip(&code);
            err = quirc_decode(&code, &data);
            if (err == QUIRC_SUCCESS) {
                ESP_LOGW(TAG, "Decode SUCCESS after Flip!");
            }
        }

        if (err == QUIRC_SUCCESS) {
            int text_y = (min_y - 20 > 0) ? (min_y - 20) : (min_y + 5);
            // 使用 frame_printf 打印字符串
            frame_printf(lcd_buf, width, height, min_x, text_y, box_color, 16, "QR: %s", (char *)data.payload);
            ESP_LOGI(TAG, "Decoded QR: %s (%dbytes)", (char *)data.payload, data.payload_len);
        } else {
            frame_printf(lcd_buf, width, height, min_x, min_y + 5, 0xF800, 16, "Decode Err");
        }
    }
}

// ==========================================
// 独立任务函数：给它分配大栈，防止栈溢出
// ==========================================
static void qr_code_task(void *arg)
{
    // 分配缓冲区
    uint16_t *lcd_buf = (uint16_t *)heap_caps_malloc(240 * 280 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    uint8_t *gray_buf = (uint8_t *)heap_caps_malloc(240 * 240, MALLOC_CAP_SPIRAM);
    if (!lcd_buf || !gray_buf) {
        ESP_LOGE(TAG, "Failed to allocate buffers!");
        vTaskDelete(NULL);
        return;
    }

    memset(lcd_buf, 0, 240 * 280 * 2);
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)arg;
    esp_lcd_panel_draw_bitmap(panel, 0, 0, 240, 280, lcd_buf);

    struct quirc *qr = quirc_new();
    if (quirc_resize(qr, 240, 240) < 0) {
        ESP_LOGE(TAG, "Failed to resize quirc");
        quirc_destroy(qr);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "QR Code Scanner Ready");

    while (1) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) continue;

        rgb565_to_grayscale(fb->buf, gray_buf, fb->width, fb->height);

        int w, h;
        uint8_t *quirc_buf = quirc_begin(qr, &w, &h);
        if (quirc_buf) {
            memcpy(quirc_buf, gray_buf, w * h);
        }
        quirc_end(qr);

        int count = quirc_count(qr);
        memcpy(lcd_buf, fb->buf, fb->len);

        if (count > 0) {
            draw_qr_result(lcd_buf, 240, 240, qr, count);
        }

        for (int i = 0; i < 6; i++) {
            esp_lcd_panel_draw_bitmap(panel, 0, 20 + i * 40, 240, 20 + (i + 1) * 40, lcd_buf + i * 40 * 240);
            vTaskDelay(pdMS_TO_TICKS(5));
        }

        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void run_qr_code_app(void)
{
    // 1. 初始化外设
    bsp_i2c_init();
    for(int i=0; i<5; i++) { if(xl9555_init_bsp() == ESP_OK) break; vTaskDelay(200); }
    ESP_ERROR_CHECK(camera_init()); 

    bsp_display_config_t disp_cfg = { .max_transfer_sz = 240 * 40 * 2 };
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
    ESP_ERROR_CHECK(bsp_display_new(&disp_cfg, &panel, &io));
    ESP_ERROR_CHECK(bsp_display_backlight_on());

    // 2. 创建独立任务来运行解码，分配 16KB 栈空间 (1024 * 16)
    xTaskCreatePinnedToCore(qr_code_task, "qr_task", 1024 * 16, (void *)panel, 5, NULL, 1);
}