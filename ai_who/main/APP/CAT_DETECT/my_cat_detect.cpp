#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "my_cat_detect.hpp"
#include "esp_lcd_panel_ops.h"
#include "bsp/esp-bsp.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

extern "C" {
    #include "bsp/camera.h"
    #include "bsp/display.h"
    #include "key.h"
}

extern "C" void frame_printf(uint16_t *frame, int width, int height, int x, int y, uint16_t color, uint8_t sizey, const char *format, ...);

static const char* TAG = "CatDetect";

void draw_cat_result(uint16_t* lcd_buf, int width, int height, const std::list<dl::detect::result_t> &results)
{
    if (results.empty()) return;

    uint16_t color = (uint16_t)GREEN;
    uint16_t rec_color = (color >> 8) | (color << 8);

    for (const auto &res : results)
    {
        int x1 = res.box[0], y1 = res.box[1], x2 = res.box[2], y2 = res.box[3];

        /* 边界保护 */
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if (x2 >= width) x2 = width - 1;
        if (y2 >= height) y2 = height - 1;

        /* 画矩形框 */
        for (uint16_t i = x1; i <= x2; i++)
        {
            lcd_buf[y1 * width + i] = rec_color;
            lcd_buf[y2 * width + i] = rec_color;
        }
        for (uint16_t i = y1; i <= y2; i++)
        {
            lcd_buf[i * width + x1] = rec_color;
            lcd_buf[i * width + x2] = rec_color;
        }
        
        /* 在框的右上角显示置信度分数信息 */
        uint16_t y0 = (y1 - 24 > 0) ? (y1 - 24) : (y1 - 2);
        frame_printf(lcd_buf, width, height, x1, y0, rec_color, 24, "Cat: %.2f", res.score);
    }
}

void run_cat_detect_app(void)
{
    vTaskPrioritySet(xTaskGetCurrentTaskHandle(), 5);

    /* 初始化外设 */
    bsp_i2c_init();
    for (int i = 0; i < 5; i++) { if (xl9555_init_bsp() == ESP_OK) break; vTaskDelay(200);}
    ESP_ERROR_CHECK(camera_init());
    key_init();

    bsp_display_config_t disp_cfg = {.max_transfer_sz = 240 * 40 * 2};
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
    ESP_ERROR_CHECK(bsp_display_new(&disp_cfg, &panel, &io));
    ESP_ERROR_CHECK(bsp_display_backlight_on());

    uint16_t* lcd_buf = (uint16_t*)heap_caps_malloc(240 * 280 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    uint8_t* modle_buf = (uint8_t*)heap_caps_aligned_alloc(4, 240 * 240 * 2, MALLOC_CAP_SPIRAM);
    if (!lcd_buf || !modle_buf)
    {
        ESP_LOGI(TAG, "Failed to allocate buffers!");
        return;
    }

    memset(lcd_buf, 0, 240 * 280 * 2);
    esp_lcd_panel_draw_bitmap(panel, 0, 0, 240, 280, lcd_buf);
    vTaskDelay(100);

    CatDetect* detector = new CatDetect(CatDetect::ESPDET_PICO_224_224_CAT, true);
    ESP_LOGI(TAG, "Cat detection modle loaded");

    while (1)
    {
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) continue;

        memcpy(modle_buf, fb->buf, fb->len);
        dl::image::img_t img = {
            .data = modle_buf,
            .width = (uint16_t)fb->width,
            .height = (uint16_t)fb->height,
            .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565BE
        };

        auto &results = detector->run(img);

        memcpy(lcd_buf, fb->buf, fb->len);

        draw_cat_result(lcd_buf, 240, 240, results);

        for (uint8_t i = 0; i < 6; i++)
        {
            esp_lcd_panel_draw_bitmap(panel, 0, 20 + i * 40, 240, 20 + (i + 1) * 40, lcd_buf + 240 * i * 40);
            vTaskDelay(5);
        }

        esp_camera_fb_return(fb);
        vTaskDelay(10);
    }
}
