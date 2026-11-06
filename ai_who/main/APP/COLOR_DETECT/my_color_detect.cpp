#include "my_color_detect.hpp"
#include "color_detect.hpp"      // 官方颜色检测头文件
#include "dl_image_draw.hpp"     // 官方画图函数头文件 (draw_hollow_rectangle)
#include "esp_camera.h"
#include "bsp/esp-bsp.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <vector>
#include <array>
#include <string>
#include <cstring>

extern "C" {
    #include "bsp/camera.h"
    #include "bsp/display.h"
}

// 复用之前的字符串打印函数
extern "C" void frame_printf(uint16_t *frame, int width, int height, int x, int y, uint16_t color, uint8_t sizey, const char *format, ...);

static const char *TAG = "ColorDetect";

// 定义任务句柄和队列
static TaskHandle_t camera_task_handle = NULL;
static TaskHandle_t ai_task_handle = NULL;
static TaskHandle_t display_task_handle = NULL;

static QueueHandle_t xQueueFrameO = NULL;       // 采集到 AI
static QueueHandle_t xQueueAIFrameO = NULL;     // AI 到 显示 (新增)

// 【重要修改】官方接口要求 hsv_min 和 hsv_max 必须是 std::array<uint8_t, 3>
// 并且要求 hsv_min[0] <= hsv_max[0]。
// 红色的 H 值通常跨越 0（例如 156~180 和 0~10）。为了兼容，我们将红色拆成两段注册。
typedef struct {
    std::array<uint8_t, 3> hsv_min;
    std::array<uint8_t, 3> hsv_max;
    uint32_t area_thresh;
    const char *name;
    uint16_t lcd_color; // 对应的 LCD 边框颜色 (RGB565)
} color_cfg_t;

static std::vector<color_cfg_t> color_cfgs = {
    // 红色拆分为两段
    {{0,   70,  90}, {10,  255, 255}, 64, "red_low",  0xF800},
    {{156, 70,  90}, {180, 255, 255}, 64, "red_high", 0xF800},
    // 其它颜色
    // {{11,  70,  90}, {22,  255, 255}, 64, "orange",   0xFD20},
    {{23,  70,  90}, {33,  255, 255}, 64, "yellow",   0xFFE0},
    {{34,  70,  90}, {75,  255, 255}, 64, "green",    0x07E0},
    // {{76,  70,  90}, {96,  255, 255}, 64, "cyan",     0x07FF},
    {{97,  70,  90}, {124, 255, 255}, 64, "blue",     0x001F},
    // {{125, 70,  90}, {155, 255, 255}, 64, "purple",   0xF81F},
    // // 黑白灰的饱和度和明度特征不同
    // {{0,   0,   220},{180, 40,  255}, 64, "white",    0xFFFF},
    // {{0,   0,   50}, {180, 50,  219}, 64, "gray",     0x8410},
    // {{0,   0,   0},  {180, 255, 45},  64, "black",    0x0000}
};

// 自定义简单的空心矩形绘制，直接操作内存，不受库版本影响
static void draw_my_rectangle(uint16_t *lcd_buf, int width, int height, int x1, int y1, int x2, int y2, uint16_t color) {
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= width) x2 = width - 1;
    if (y2 >= height) y2 = height - 1;
    
    for (int x = x1; x <= x2; x++) {
        lcd_buf[y1 * width + x] = color;
        lcd_buf[y2 * width + x] = color;
    }
    for (int y = y1; y <= y2; y++) {
        lcd_buf[y * width + x1] = color;
        lcd_buf[y * width + x2] = color;
    }
}

// 专门的摄像头采集任务
static void color_camera_task(void *arg)
{
    camera_fb_t *fb = NULL;
    while (1) {
        // 获取摄像头图像
        fb = esp_camera_fb_get();
        if (fb) {
            // 将图像指针通过队列发送给 AI 任务
            // 如果队列满了，说明 AI 处理不过来，这里会阻塞等待
            if (xQueueSend(xQueueFrameO, &fb, portMAX_DELAY) != pdPASS) {
                // 发送失败（极少见），必须把 fb 还回去
                esp_camera_fb_return(fb);
            }
        }
    }
}

static void color_detect_task(void* arg)
{
    uint16_t x1, y1, x2, y2;
    int w, h, area;
    camera_fb_t *fb = NULL;

    // 实例化颜色检测器 (传入宽高，必须是 240x240)
    ColorDetect detector(240, 240);
    
    // 注册颜色
    for (const auto &cfg : color_cfgs) {
        detector.register_color(cfg.hsv_min, cfg.hsv_max, cfg.name, cfg.area_thresh);
    }
    
    // 可选：开启形态学滤波(腐蚀/膨胀)，有助于去除噪点，让框更平滑，使用 10×10 的核，数值越大过滤越狠
    // detector.enable_morphology(10); /* 开启这个画面会变卡 */

    ESP_LOGI(TAG, "Color Detector Ready. Registered %d colors.", detector.get_color_num());

    // 主循环
    while (1) {
        // 【核心修改】从队列中获取图像，如果没有图，就一直死等
        if (xQueueReceive(xQueueFrameO, &fb, portMAX_DELAY) == pdPASS) {
            
            // 1. 准备图像结构体
            dl::image::img_t lcd_img = {
                .data = fb->buf,
                .width = 240,
                .height = 240,
                .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565BE,
            };

            // 2. 执行检测
            std::list<dl::detect::result_t> &results = detector.run(lcd_img);

            // 3. 筛选面积最大的 2 个框
            const int MAX_DRAW_BOXES = 2;
            std::vector<dl::detect::result_t> top_results;

            for (const auto &res : results) {
                x1 = res.box[0]; x2 = res.box[2];
                y1 = res.box[1]; y2 = res.box[3];
                w = x2 - x1;
                h = y2 - y1;
                area = w * h;

                // 除去面积小于500和大于25000的物体
                if (area < 500 || area > 25000) continue; 

                // 除去细长的物体
                float aspect_ratio = (float)(x2 - x1) / (y2 - y1);
                if (aspect_ratio < 0.3 || aspect_ratio > 3.0) continue; 

                top_results.push_back(res);
            }

            // 降序排列
            std::sort(top_results.begin(), top_results.end(), 
                [](const dl::detect::result_t &a, const dl::detect::result_t &b) {
                    int areaA = (a.box[2] - a.box[0]) * (a.box[3] - a.box[1]);
                    int areaB = (b.box[2] - b.box[0]) * (b.box[3] - b.box[1]);
                    return areaA > areaB; 
                });

            int draw_count = (top_results.size() < MAX_DRAW_BOXES) ? top_results.size() : MAX_DRAW_BOXES;

            // 4. 遍历绘制
            for (int i = 0; i < draw_count; i++) {
                const auto &res = top_results[i];
                int color_idx = res.category;
                if (color_idx < 0 || color_idx >= color_cfgs.size()) continue;

                const char *color_name = color_cfgs[color_idx].name;
                uint16_t box_color = color_cfgs[color_idx].lcd_color;

                std::vector<uint8_t> color_bytes = {
                    (uint8_t)(box_color >> 8),
                    (uint8_t)(box_color & 0xFF)
                };

                int bx1 = res.box[0], by1 = res.box[1];
                int bx2 = res.box[2], by2 = res.box[3];

                dl::image::draw_hollow_rectangle(lcd_img, bx1, by1, bx2, by2, color_bytes, 2);

                int text_y = (by1 - 16 > 0) ? (by1 - 16) : (by1 + 2);
                const char *display_name = (strncmp(color_name, "red", 3) == 0) ? "red" : color_name;
                frame_printf((uint16_t*)fb->buf, 240, 240, bx1, text_y, box_color, 16, "%s", display_name);
            }

            // 【关键修改】在每次处理完一帧后，让出 1~2ms 给空闲任务（IDLE0）喘息，此时方便及时喂狗
            vTaskDelay(pdMS_TO_TICKS(2));

            // 【核心修改】画完框后，不推屏了，直接丢给显示队列，继续拿下一张！
            if (xQueueSend(xQueueAIFrameO, &fb, portMAX_DELAY) != pdPASS) {
                esp_camera_fb_return(fb); // 如果发不出去（如队列满），只能还回去防泄漏
            }
        }
    }
}

static void color_display_task(void* arg)
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
        // 从 AI 任务获取画好框的图像
        if (xQueueReceive(xQueueAIFrameO, &fb, portMAX_DELAY) == pdPASS) {
            
            // 分 6 块推送到屏幕
            for (int i = 0; i < 6; i++) {
                esp_lcd_panel_draw_bitmap(panel, 0, 20 + i * 40, 240, 20 + (i + 1) * 40, (uint16_t*)fb->buf + i * 40 * 240);
                // 注意：这里加上 1~2ms 延时是必要的，否则 SPI DMA 可能会丢数据导致屏幕闪烁
                vTaskDelay(pdMS_TO_TICKS(1)); 
            }

            // 【关键】推屏完成后，这块内存才真正用完，在这里还给摄像头驱动
            esp_camera_fb_return(fb);
        }
    }
}

void run_color_detect_app(void)
{
    vTaskPrioritySet(xTaskGetCurrentTaskHandle(), 5);

    // 1. 初始化外设
    bsp_i2c_init();
    for(int i=0; i<5; i++) { if(xl9555_init_bsp() == ESP_OK) break; vTaskDelay(200); }
    ESP_ERROR_CHECK(camera_init()); 

    bsp_display_config_t disp_cfg = { .max_transfer_sz = 240 * 40 * 2 };
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
    ESP_ERROR_CHECK(bsp_display_new(&disp_cfg, &panel, &io));
    ESP_ERROR_CHECK(bsp_display_backlight_on());

    // 2. 创建两个消息队列
    // xQueueFrameO: 原图队列，深度 5
    xQueueFrameO = xQueueCreate(5, sizeof(camera_fb_t *));
    // xQueueAIFrameO: 处理后的图队列，深度 5
    xQueueAIFrameO = xQueueCreate(5, sizeof(camera_fb_t *));

    if (xQueueFrameO == NULL || xQueueAIFrameO == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        return;
    }

    // 3. 创建三个任务
    // 采集任务 (绑核 1)
    xTaskCreatePinnedToCore(color_camera_task, "cam_task", 4 * 1024, NULL, 5, &camera_task_handle, 1);
    
    // AI 任务 (绑核 1)
    xTaskCreatePinnedToCore(color_detect_task, "ai_task", 10 * 1024, NULL, 4, &ai_task_handle, 1);
    
    // 显示任务 (绑核 0)
    xTaskCreatePinnedToCore(color_display_task, "disp_task", 4 * 1024, (void*)panel, 6, &display_task_handle, 0);
}


