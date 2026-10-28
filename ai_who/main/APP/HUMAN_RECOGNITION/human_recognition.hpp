#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define FRAME_WIDTH     240
#define FRAME_HEIGHT    240

// 状态定义
typedef enum {
    APP_STATE_DETECT,     // 仅检测
    APP_STATE_ENROLL,     // 录入 (长按)
    APP_STATE_RECOGNIZE,  // 识别 (短按)
    APP_STATE_DELETE      // 删除 (双击)
} app_state_t;

/**
 * @brief 启动人脸识别应用
 */
void run_human_recognition_app(void);

#ifdef __cplusplus
}
#endif