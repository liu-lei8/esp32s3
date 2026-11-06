#define private public  /* 破防宏：C++ 调试中经常用的“黑科技”：宏替换强破私有权限。 */
#include "human_face_recognition.hpp"
#include "dl_recognition_database.hpp"
#undef private

#include "human_recognition.hpp"
#include "human_face_detect.hpp"
#include "esp_camera.h"
#include "bsp/esp-bsp.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_vfs_fat.h"
#include <stdarg.h>

extern "C" {
    #include "bsp/camera.h"
    #include "bsp/display.h"
    #include "key.h" 
}

static const char *TAG = "FaceRec";

// 挂载 FATFS 并返回路径
static std::string mount_fr_partition(void) {
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = false,
        .use_one_fat = false
    };
    wl_handle_t wl_handle;
    // 挂载分区名为 "fr"，挂载点为 "/fr"
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl("/fr", "fr", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return "";
    }
    ESP_LOGI(TAG, "FATFS mounted at /fr");
    return "/fr/face_db.bin";       /* 人脸特征存储到这个文件里 */
}

// 绘制人脸框和关键点
static void draw_face_result(uint16_t *lcd_buf, int width, int height, const std::list<dl::detect::result_t> &results, bool recognized = false)
{
    if (results.empty()) return;
    
    // 如果识别成功用绿框，否则用蓝框
    uint16_t rec_color = recognized ? 0xE007 : 0x00F8; 
    uint16_t point_color = 0x1F00; // 红色点

    for (const auto &res : results) {
        int x1 = res.box[0], y1 = res.box[1], x2 = res.box[2], y2 = res.box[3];
        if (x1 < 0) x1 = 0; 
        if (y1 < 0) y1 = 0;
        if (x2 > width) x2 = width - 1; 
        if (y2 > height) y2 = height - 1;
        
        for (int i = x1; i <= x2; i++) {
            lcd_buf[y1 * width + i] = rec_color;
            lcd_buf[y2 * width + i] = rec_color;
        }
        for (int j = y1; j <= y2; j++) {
            lcd_buf[j * width + x1] = rec_color;
            lcd_buf[j * width + x2] = rec_color;
        }

        if (res.keypoint.size() >= 10) {
            for (uint8_t i = 0; i < 5; i++) {
                int rx = res.keypoint[i * 2];
                int ry = res.keypoint[i * 2 + 1];
                for (int8_t y = -2; y <= 2; y++) {
                    for (int8_t x = -2; x <= 2; x++) {
                        int px = rx + x, py = ry + y;
                        if (px >= 0 && px < width && py >= 0 && py < height) {
                            lcd_buf[py * width + px] = point_color;
                        }
                    }
                }
            }
        }
    }
}

// 高级按键状态机（支持短按、长按、双击）
app_state_t get_key_event() {
    static uint32_t press_start_time = 0;
    static uint32_t release_time = 0;
    static uint8_t is_pressing = 0;
    static uint8_t click_count = 0;
    
    uint8_t key_val = key_scan(1); // 支持连续按下模式
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;    /* 关键：now是实时更新的 */

    if (key_val == BOOT_PRES) { // 按下
        if (!is_pressing) {
            is_pressing = 1;
            press_start_time = now;
        }
    } else { // 松开
        if (is_pressing) {
            is_pressing = 0;
            uint32_t duration = now - press_start_time;
            
            if (duration > 1000) {
                click_count = 0; // 长按触发，清空点击次数
                return APP_STATE_ENROLL; // 长按 > 1秒：录入
            } else if (duration > 50) {
                click_count++;
                release_time = now;
            }
        }
    }

    // 检查双击 (500ms内按了2次)
    if (click_count > 0 && !is_pressing && (now - release_time > 500)) {
        if (click_count == 1) {
            click_count = 0;
            return APP_STATE_RECOGNIZE; // 短按：识别
        } else if (click_count >= 2) {
            click_count = 0;
            return APP_STATE_DELETE; // 双击：删除
        }
    }

    return APP_STATE_DETECT; 
}

/**
 * @brief 在检测到的人脸帧上显示对应登记、识别和删除的状态
 * @param lcd_buf: 输入处理然后输出的帧
 * @param real_id: 实际的链表迭代器的随机id，而不是顺序索引
 * @param state：要显示的登记、识别和删除的状态
 */
static void frame_show_state(uint16_t* lcd_buf, uint16_t real_id, app_state_t state)
{
    int x = 20;
    int y = 50;

    switch (state)
    {
        case APP_STATE_ENROLL:
            frame_printf(lcd_buf, FRAME_WIDTH, FRAME_HEIGHT, x, y, BLUE, 32, "Enroll successful !");
            break;
        case APP_STATE_RECOGNIZE:
            if (real_id > 0)
            {
                frame_printf(lcd_buf, FRAME_WIDTH, FRAME_HEIGHT, x, y, GREEN, 32, "Rec: %dID", real_id);
            }
            else frame_printf(lcd_buf, FRAME_WIDTH, FRAME_HEIGHT, x, y, GREEN, 32, "Who ?");
            break;
        case APP_STATE_DELETE:
            if (real_id > 0)
            {
                frame_printf(lcd_buf, FRAME_WIDTH, FRAME_HEIGHT, x, y, RED, 32, "Delete: %dID", real_id);
            }
            else frame_printf(lcd_buf, FRAME_WIDTH, FRAME_HEIGHT, x, y, RED, 32, "No data", real_id);
            break;
        default: break;
    }
}

void run_human_recognition_app(void)
{
    uint16_t real_id = 0;

    // 1. 挂载存储分区
    std::string db_path = mount_fr_partition();
    if (db_path.empty()) {
        ESP_LOGE(TAG, "Failed to mount DB partition. Aborting.");
        return;
    }

    // 2. 初始化硬件
    bsp_i2c_init();
    for(int i=0; i<5; i++) { if(xl9555_init_bsp() == ESP_OK) break; vTaskDelay(200); }
    ESP_ERROR_CHECK(camera_init());
    key_init(); 

    bsp_display_config_t disp_cfg = { .max_transfer_sz = 240 * 40 * 2 };
    esp_lcd_panel_handle_t panel;
    esp_lcd_panel_io_handle_t io;
    ESP_ERROR_CHECK(bsp_display_new(&disp_cfg, &panel, &io));
    ESP_ERROR_CHECK(bsp_display_backlight_on());

    uint16_t *lcd_buf = (uint16_t *)heap_caps_malloc(240 * 280 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    uint8_t *model_buf = (uint8_t *)heap_caps_aligned_alloc(4, 240 * 240 * 2, MALLOC_CAP_SPIRAM);
    
    memset(lcd_buf, 0, 240 * 280 * 2);
    esp_lcd_panel_draw_bitmap(panel, 0, 0, 240, 280, lcd_buf);
    vTaskDelay(pdMS_TO_TICKS(100));

    bsp_display_show_string(50, 50, "ESP32-S3", RED, WHITE, 32, 1);
    vTaskDelay(pdMS_TO_TICKS(3000));

    // 3. 初始化检测器与识别器
    HumanFaceDetect *detector = new HumanFaceDetect(HumanFaceDetect::MSRMNP_S8_V1, true);
    
    // 初始化识别器，传入挂载点路径,默认阈值 0.5 (相似度)
    HumanFaceRecognizer *recognizer = new HumanFaceRecognizer(db_path, HumanFaceFeat::MFN_S8_V1, true); 
    
    ESP_LOGI(TAG, "Recognition Model Loaded. Enrolled faces: %d", recognizer->get_num_feats());
    ESP_LOGI(TAG, "System Ready. Short Press = Recognize, Long Press = Enroll, Double Click = Delete Last.");

    // 4. 主循环
    while (1) {
        // 获取按键状态 (如果是 DETECT，说明没有按键触发)
        app_state_t key_event = get_key_event();
        
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) continue;
        
        memcpy(model_buf, fb->buf, fb->len);
        dl::image::img_t img = {
            .data = model_buf,
            .width = (uint16_t)fb->width,
            .height = (uint16_t)fb->height,
            .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565BE,
        };

        // 人脸检测
        auto &detect_results = detector->run(img);
        memcpy(lcd_buf, fb->buf, fb->len); // 把画面拷入LCD缓冲
        
        bool is_recognized = false;

        if (!detect_results.empty()) {
            // 如果检测到人脸，根据按键事件执行对应操作
            if (key_event == APP_STATE_ENROLL) {
                ESP_LOGW(TAG, "Enrolling new face...");
                esp_err_t err = recognizer->enroll(img, detect_results);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Enroll Success! Total faces: %d", recognizer->get_num_feats());
                } else {
                    ESP_LOGE(TAG, "Enroll Failed!");
                }
            } 
            else if (key_event == APP_STATE_RECOGNIZE) {
                ESP_LOGW(TAG, "Recognizing face...");
                auto rec_results = recognizer->recognize(img, detect_results);
                
                if (!rec_results.empty()) {
                    // 取相似度最高的一个（通常就是第一个）
                    int idex = rec_results.front().id;
                    float similarity = rec_results.front().similarity;

                    /* 在 C++ 中，如果要让链表迭代器向前移动 N 步，必须使用 std::advance 或者 std::next。 */
                    auto it = std::next(recognizer->m_db->m_feats.begin(), idex - 1);
                    real_id = it->id;
                    
                    if (idex > 0) {
                        ESP_LOGI(TAG, "Match! Index: %d real_id: %d, Similarity: %.2f", idex, real_id, similarity);
                        is_recognized = true;
                    } else {
                        ESP_LOGI(TAG, "Unknown Face! Similarity: %.2f", similarity);
                    }
                } else {
                    ESP_LOGE(TAG, "Recognition Failed.");
                }
            }
            else if (key_event == APP_STATE_DELETE) {
                auto rec_results = recognizer->recognize(img, detect_results);

                if (!rec_results.empty()){
                    int idex = rec_results.front().id;

                    // 【核心修改】：从识别器的内部数据库中取出真实 id
                    // 因为我们已经破防了，所以可以直接访问 recognizer->m_db->m_feats
                    if (recognizer->m_db && idex > 0 && idex <= recognizer->m_db->m_feats.size()) {
                        
                        // 假设 recognize 返回的 idex 是排名 (1 对应索引 0)
                        auto it = recognizer->m_db->m_feats.begin();
                        std::advance(it, idex - 1);
                        real_id = it->id;
                        
                        ESP_LOGW(TAG, "Deleting Idex:%d face... real_id:%d", idex, real_id);
                        
                        // 用真实的 id 去删，绝对能删掉
                        esp_err_t err = recognizer->delete_feat(real_id);

                        if (err == ESP_OK) {
                            ESP_LOGI(TAG, "Delete Success! Remaining faces: %d", recognizer->get_num_feats());
                        } else {
                            ESP_LOGE(TAG, "Delete Failed!");
                        }
                    } else {
                        ESP_LOGE(TAG, "Invalid matched_id or db not initialized.");
                    }
                } else {
                    ESP_LOGE(TAG, "Unknown Face!");
                }
            }

            frame_show_state(lcd_buf, real_id, key_event);
            real_id = 0;
            // 绘制框和点 (如果识别成功，框会变成绿色)
            draw_face_result(lcd_buf, 240, 240, detect_results, is_recognized);
        }

        // 分块显示
        for (int i = 0; i < 6; i++) {
            esp_lcd_panel_draw_bitmap(panel, 0, 20 + i * 40, 240, 20 + (i + 1) * 40, lcd_buf + i * 40 * 240);
            vTaskDelay(pdMS_TO_TICKS(5));
        }

        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
