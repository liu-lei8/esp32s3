#pragma once  /* my_cat_detect.hpp*/

#include <stdint.h>
#include "cat_detect.hpp"

#ifdef __cplusplus
extern "C" {
#endif

void run_cat_detect_app(void);

#ifdef __cplusplus
}
#endif

/**
 * @brief 绘制猫脸框（不需要关键点）
 */
void draw_cat_result(uint16_t* lcd_buf, int width, int height, const std::list<dl::detect::result_t> &results);