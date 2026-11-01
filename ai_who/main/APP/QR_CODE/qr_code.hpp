#pragma once

#include <stdint.h>
#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动二维码识别应用
 */
void run_qr_code_app(void);

#ifdef __cplusplus
}
#endif