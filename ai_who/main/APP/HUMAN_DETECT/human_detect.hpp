#pragma once

#include <stdint.h>
#include <list>
#include "human_face_detect.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动人脸检测应用（包含初始化与死循环）
 */
void run_human_detect_app(void);

#ifdef __cplusplus
}
#endif

/**
 * @brief 在 LCD 缓冲区上绘制人脸框和关键点
 * @param lcd_buf LCD 显存缓冲区指针
 * @param width 图像宽度
 * @param height 图像高度
 * @param results 人脸检测结果
 */
void draw_face_result(uint16_t *lcd_buf, int width, int height, 
                      const std::list<dl::detect::result_t> &results);