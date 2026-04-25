#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/rmt_rx.h"
#include "driver/rmt_tx.h"
#include "lcd.h"
#include "ir_nec_encoder.h"

#define RMT_IN_GPIO_PIN         GPIO_NUM_2      /*连接RMT_RX_IN的GPIO端口*/
#define RMT_OUT_GPIO_PIN        GPIO_NUM_4      /*连接RMT_TX_OUT的GPIO端口*/
#define RMT_RESOLUTION_HZ       1000000         /*1MHz频率，1tick = 1us*/
#define RMT_NEC_DECODE_MARGIN   200             /* 判断NEC时序时长的容差值，小于（值+此值），大于（值-此值）为正确 */

/*NEC协议时序时间，协议头9.5ms 4.5ms，逻辑0两个电平时常，逻辑1两个电平时长，重复码两个电平时长*/
/*在NEC协议中，同步码头，以及逻辑0和逻辑1所对应的高低电平就是一个“符号”*/
#define NEC_LEADING_CODE_DURATION_0     9000
#define NEC_LEADING_CODE_DURATION_1     4500
#define NEC_PAYLOAD_ZERO_DURATION_0     560
#define NEC_PAYLOAD_ZERO_DURATION_1     560
#define NEC_PAYLOAD_ONE_DURATION_0      560
#define NEC_PAYLOAD_ONE_DURATION_1      1690
#define NEC_REPEAT_CODE_DURATION_0      9000
#define NEC_REPEAT_CODE_DURATION_1      2250

extern uint16_t s_nec_code_address;
extern uint16_t s_nec_code_command;

extern QueueHandle_t receive_queue;
extern rmt_channel_handle_t rx_channel;
extern rmt_channel_handle_t tx_channel;
extern rmt_symbol_word_t raw_symbols[64];
extern rmt_receive_config_t receive_config;
extern rmt_transmit_config_t transmit_config;

extern rmt_encoder_handle_t nec_encoder;
extern ir_nec_scan_code_t scan_code;

esp_err_t rmt_nec_rx_init(void);
esp_err_t rmt_nec_tx_init(void);

/**
 * @brief 打印接收到遥控器发送的红外地址和数据打印出啦并显示到lcd
 * @param rmt_nec_symbols 接收到的红外符号指针
 * @param num_symbols     接收到的红外符号总数
 * @return 空
 */
void rmt_rx_scan(rmt_symbol_word_t* rmt_nec_symbols, size_t num_symbols);

/**
 * @brief 该函数打印RMT自发自收的红外数据
 */
void rmt_rx_scan1(rmt_symbol_word_t* rmt_nec_symbols, size_t num_symbols);