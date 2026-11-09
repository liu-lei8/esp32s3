#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include "my_motion_detect.hpp"
#include "esp_camera.h"
#include "bsp/esp-bsp.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dl_image_process.hpp"
#include "dl_image_draw.hpp"

extern "C" {
    #include "bsp/camera.h"
    #include "bsp/display.h"
}


static const char *TAG = "MotionDetect";

// 全局任务句柄与队列
static TaskHandle_t camera_task_handle = NULL;
static TaskHandle_t ai_task_handle = NULL;
static TaskHandle_t display_task_handle = NULL;

static QueueHandle_t xQueueFrameO = NULL;    // 采集 -> AI
static QueueHandle_t xQueueAIFrameO = NULL;  // AI -> 显示

// 运动侦测参数
#define MOTION_STRIDE       8     // 检测点间隔（越小越灵敏，但越慢）
#define MOTION_THRESHOLD    15    // 单点阈值（像素差超过此值才算“变化”）
#define MOTION_POINT_LIMIT  10    // 变化点总数阈值（超过此值判定为“有运动”）

// 手写实心矩形绘制（替代官方库被删的接口）
static void my_draw_filled_rectangle(dl::image::img_t &img, int x, int y, int w, int h, uint16_t color) {
    uint16_t *buf = (uint16_t *)img.data;
    uint16_t color_swap = (color >> 8) | (color << 8);
    // 简单的边界保护
    if (x < 0 || y < 0 || x + w > img.width || y + h > img.height) return;
    
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            buf[i * img.width + j] = color_swap;
        }
    }
}

// 手写帧差法运动侦测（替代官方库被删的 get_moving_point_number）
static uint32_t my_get_moving_point_number(const dl::image::img_t &img1, const dl::image::img_t &img2, int stride, int threshold) {
    uint32_t moving_points = 0;
    uint16_t *p1 = (uint16_t *)img1.data;
    uint16_t *p2 = (uint16_t *)img2.data;

    // 每隔 stride 个点比较一次，降低计算量
    for (int y = 0; y < img1.height; y += stride) {
        for (int x = 0; x < img1.width; x += stride) {
            int idx = y * img1.width + x;
            uint16_t c1 = (p1[idx] >> 8) | (p1[idx] << 8);
            uint16_t c2 = (p2[idx] >> 8) | (p2[idx] << 8);
            
            // 简单的颜色差值计算 (提取 R, G, B)
            int r1 = (c1 >> 11) & 0x1F, r2 = (c2 >> 11) & 0x1F;
            int g1 = (c1 >> 5) & 0x3F,  g2 = (c2 >> 5) & 0x3F;
            int b1 = c1 & 0x1F,         b2 = c2 & 0x1F;

            // 只要 RGB 中有一个通道的变化超过阈值，就算运动点
            if (abs(r1 - r2) > threshold || abs(g1 - g2) > threshold || abs(b1 - b2) > threshold) {
                moving_points++;
            }
        }
    }
    return moving_points;
}

// ==================== 摄像头采集任务 ====================
static void motion_camera_task(void *arg)
{
    camera_fb_t *fb = NULL;
    while (1) {
        fb = esp_camera_fb_get();
        if (fb) {
            if (xQueueSend(xQueueFrameO, &fb, portMAX_DELAY) != pdPASS) {
                esp_camera_fb_return(fb);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // 主动让出 CPU，防止饿死 IDLE
    }
}

// ==================== AI 运动侦测任务 ====================
static void motion_detect_task(void *arg)
{
    camera_fb_t *fb_current = NULL;
    uint8_t* buf_prev;
    bool has_previous = false;
    int motion_counter = 0; 

    buf_prev = (uint8_t*)heap_caps_malloc(240 * 240 * 2, MALLOC_CAP_SPIRAM);
    if (!buf_prev)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for buf_prev");
    }

    while (1) {
        // 1. 获取当前帧
        if (xQueueReceive(xQueueFrameO, &fb_current, portMAX_DELAY) != pdPASS) {
            continue;
        }

        // 2. 构造新版 img_t
        dl::image::img_t img_current = {
            .data = fb_current->buf,
            .width = (uint16_t)fb_current->width,
            .height = (uint16_t)fb_current->height,
            .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565BE,
        };

        bool moved = false;

        if (has_previous) {
            dl::image::img_t img_previous = {
                .data = buf_prev,
                .width = (uint16_t)fb_current->width,
                .height = (uint16_t)fb_current->height,
                .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565BE,
            };

            // 3. 核心：调用运动侦测函数
            uint32_t moving_point_number = my_get_moving_point_number(img_previous, img_current, MOTION_STRIDE, MOTION_THRESHOLD);

            if (moving_point_number > MOTION_POINT_LIMIT) {
                moved = true;
                ESP_LOGI(TAG, "Something moved! (points: %lu)", moving_point_number);
            } else {
                ESP_LOGD(TAG, "Something not moved (points: %lu)", moving_point_number);
            }
        }

        // 4. 保存当前帧作为下一轮的“上一帧”
        memcpy(buf_prev, fb_current->buf, 240 * 240 * 2);
        has_previous = true;

        // 5. 如果检测到运动，在画面上画一个 40x40 的实心矩形作为提示
        if (moved) {
            // 由于我们用了大端 RGB565BE，红色的像素值为 0x00F8
            my_draw_filled_rectangle(img_current, 0, 0, 40, 40, RED); 
        }

        // 6. 将当前帧发送给显示任务
        if (xQueueSend(xQueueAIFrameO, &fb_current, portMAX_DELAY) != pdPASS) {
            esp_camera_fb_return(fb_current);
        }

        // 主动让出 CPU，防止看门狗触发
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// ==================== LCD 显示任务 ====================
static void motion_display_task(void *arg)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)arg;
    camera_fb_t *fb = NULL;

    // 清屏
    uint16_t *init_buf = (uint16_t *)heap_caps_malloc(240 * 40 * 2, MALLOC_CAP_DMA);
    if (init_buf) {
        memset(init_buf, 0, 240 * 40 * 2);
        for (int i = 0; i < 7; i++) {
            esp_lcd_panel_draw_bitmap(panel, 0, i * 40, 240, (i + 1) * 40, init_buf);
        }
        heap_caps_free(init_buf);
    }

    while (1) {
        if (xQueueReceive(xQueueAIFrameO, &fb, portMAX_DELAY) == pdPASS) {
            for (int i = 0; i < 6; i++) {
                esp_lcd_panel_draw_bitmap(panel, 0, 20 + i * 40, 240, 20 + (i + 1) * 40,
                                          (uint16_t *)fb->buf + i * 40 * 240);
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            vTaskDelay(pdMS_TO_TICKS(5));
            esp_camera_fb_return(fb);
        }
    }
}

// ==================== 应用启动入口 ====================
void run_motion_detect_app(void)
{
    // 1. 初始化外设
    bsp_i2c_init();
    for (int i = 0; i < 5; i++) {
        if (xl9555_init_bsp() == ESP_OK) break;
        vTaskDelay(200);
    }
    ESP_ERROR_CHECK(camera_init());

    bsp_display_config_t disp_cfg = { .max_transfer_sz = 240 * 40 * 2 };
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
    ESP_ERROR_CHECK(bsp_display_new(&disp_cfg, &panel, &io));

    if (bsp_display_backlight_on() != ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(100));
        bsp_display_backlight_on();
    }

    // 2. 创建队列
    xQueueFrameO = xQueueCreate(5, sizeof(camera_fb_t *));
    xQueueAIFrameO = xQueueCreate(5, sizeof(camera_fb_t *));
    if (!xQueueFrameO || !xQueueAIFrameO) {
        ESP_LOGE(TAG, "Failed to create queues");
        return;
    }

    // 3. 创建三个任务
    xTaskCreatePinnedToCore(motion_camera_task, "motion_cam", 4 * 1024, NULL, 5, &camera_task_handle, 1);
    xTaskCreatePinnedToCore(motion_detect_task, "motion_ai", 10 * 1024, NULL, 4, &ai_task_handle, 1);
    xTaskCreatePinnedToCore(motion_display_task, "motion_disp", 4 * 1024, (void *)panel, 6, &display_task_handle, 0);

    ESP_LOGI(TAG, "Motion Detection Started");
}