#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

#define ADC_CHAN    ADC_CHANNEL_6   /*对应GPIO7*/
#define LOST_VAL    1               /*模拟信号转数字信号降噪，要丢掉的首尾两端的极值*/

void adc_init(void);

/**
 * @brief       获取ADC转换且进行多次采样后排序去除最高和最低值再做均值滤波后的结果
 * @note        ESP32P4 ADC对噪声敏感,可能导致ADC读数出现较大偏差
 * @note        软件上:可通过多次采样进一步降低噪声影响;硬件上:可加旁路电容连在在ADC使用引脚上
 * @param       ch      : 通道号, 0~9
 * @param       times   : 获取次数
 * @retval      通道ch的times次转换结果平均值
 */
uint16_t adc_get_result_average(adc_channel_t ch, uint32_t times);
