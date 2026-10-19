#include "human_detect.hpp"
#include "esp_camera.h"
#include "bsp/esp-bsp.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
    #include "bsp/camera.h"
    #include "bsp/display.h"
}

static const char *TAG = "FaceDetect";

// ============================================================
// 绘制人脸框和关键点
// ============================================================
void draw_face_result(uint16_t *lcd_buf, int width, int height, 
                      const std::list<dl::detect::result_t> &results)
{
    if (results.empty()) return;
    ESP_LOGI(TAG, "Detected %d face(s)", results.size());

    for (const auto &res : results) {
        int x1 = res.box[0], y1 = res.box[1], x2 = res.box[2], y2 = res.box[3];
        
        // 1. 画红色矩形框
        if (x1 < 0) x1 = 0; 
        if (y1 < 0) y1 = 0;
        if (x2 >= width) x2 = width - 1; 
        if (y2 >= height) y2 = height - 1;
        
        for (int x = x1; x <= x2; x++) {
            lcd_buf[y1 * width + x] = 0x00F8; // 红色
            lcd_buf[y2 * width + x] = 0x00F8;
        }
        for (int y = y1; y <= y2; y++) {
            lcd_buf[y * width + x1] = 0x00F8;
            lcd_buf[y * width + x2] = 0x00F8;
        }

        // 2. 绘制5个关键点 (绿色)
        if (res.keypoint.size() >= 10) {
            const uint16_t point_color = 0xE007; // 绿色
            int dot_radius = 2; 

            for (int i = 0; i < 5; i++) {
                int cx = res.keypoint[i * 2];     
                int cy = res.keypoint[i * 2 + 1]; 

                if (cx < 0 || cx >= width || cy < 0 || cy >= height) continue;

                for (int dy = -dot_radius; dy <= dot_radius; dy++) {
                    for (int dx = -dot_radius; dx <= dot_radius; dx++) {
                        int px = cx + dx;
                        int py = cy + dy;
                        if (px >= 0 && px < width && py >= 0 && py < height) {
                            lcd_buf[py * width + px] = point_color;
                        }
                    }
                }
            }
            // 可选：在串口打印关键点坐标，方便调试
            ESP_LOGI(TAG, "Landmarks: LEye[%d,%d] Nose[%d,%d] REye[%d,%d]", 
                    res.keypoint[0], res.keypoint[1],
                    res.keypoint[4], res.keypoint[5],
                    res.keypoint[6], res.keypoint[7]);
        }
    }
}

// ============================================================
// 应用主逻辑（初始化 + 主循环）
// ============================================================
void run_human_detect_app(void)
{
    vTaskPrioritySet(xTaskGetCurrentTaskHandle(), 5);

    // 1. 初始化 IO 扩展和摄像头
    bsp_i2c_init(); 
    bool xl_ok = false;
    for (int i = 0; i < 5; i++) {
        if (xl9555_init_bsp() == ESP_OK) {
            xl_ok = true;
            break;
        }
        ESP_LOGW(TAG, "XL9555 init retry %d", i + 1);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    if (!xl_ok) {
        ESP_LOGE(TAG, "XL9555 init failed after retries. Check wiring!");
        return;
    }

    ESP_ERROR_CHECK(camera_init());

    // 2. 初始化 LCD
    bsp_display_config_t disp_cfg = { .max_transfer_sz = 240 * 40 * 2 };
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
    ESP_ERROR_CHECK(bsp_display_new(&disp_cfg, &panel, &io));
    ESP_ERROR_CHECK(bsp_display_backlight_on());

    const int CHUNK_ROWS = 40;
    uint16_t *lcd_buf = (uint16_t *)heap_caps_malloc(240 * 240 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    
    // 清屏
    for (int i = 0; i < 280 / CHUNK_ROWS; i++) {
        memset(lcd_buf, 0, 240 * CHUNK_ROWS * 2);
        esp_lcd_panel_draw_bitmap(panel, 0, i * CHUNK_ROWS, 240, (i + 1) * CHUNK_ROWS, lcd_buf);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // 3. 初始化人脸检测模型
    // 第二个参数改成 true (延迟加载)
    HumanFaceDetect *detector = new HumanFaceDetect(HumanFaceDetect::MSRMNP_S8_V1, true);
    ESP_LOGI(TAG, "Face Detection Model Loaded");

    // 4. 分配模型输入缓冲
    // 改用 4 字节对齐分配，并放在 PSRAM 里，以便于esp_dl处理
    uint8_t *model_buf = (uint8_t *)heap_caps_aligned_alloc(4, 240 * 240 * 2, MALLOC_CAP_SPIRAM);
    if (!model_buf) {
        ESP_LOGE(TAG, "Failed to alloc model_buf in SPI RAM");
        return;
    }

    // 5. 主循环
    while (1) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) continue;

        // 推理数据处理
        memcpy(model_buf, fb->buf, fb->len);
        dl::image::img_t img = {
            .data = model_buf,
            .width = (uint16_t)fb->width,
            .height = (uint16_t)fb->height,
            .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565BE,
        };

        auto &results = detector->run(img);

        // 准备显示数据
        uint16_t *src = (uint16_t *)fb->buf;
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < 240; x++) {
                lcd_buf[y * 240 + x] = src[y * 240 + x];
            }
        }

        // 调用独立封装的绘制函数
        draw_face_result(lcd_buf, 240, 240, results);

        // 分块推送到屏幕
        for (int chunk = 0; chunk < 240 / CHUNK_ROWS; chunk++) {
            esp_lcd_panel_draw_bitmap(panel, 0, 20 + chunk * CHUNK_ROWS, 240, 20 + (chunk + 1) * CHUNK_ROWS, lcd_buf + chunk * CHUNK_ROWS * 240);
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        
        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}
