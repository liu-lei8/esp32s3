#pragma once /*myi2s.h*/

#include "driver/gpio.h"
#include "driver/i2s_std.h"

#define I2S_NUM                 I2S_NUM_0               /*I2S port*/
#define I2S_BCK_IO              GPIO_NUM_46             /*ES8311_SCLK位时钟信号*/
#define I2S_WS_IO               GPIO_NUM_4              /*ES8311_LRCK左右时钟信号，对应采样频率*/
#define I2S_DO_IO               GPIO_NUM_10             /*ES8311_SDOUT数据输出引脚*/
#define I2S_DI_IO               GPIO_NUM_14             /*ES8311_SDIN数据输入引脚*/
#define I2S_MCK_IO              GPIO_NUM_3              /*ES8311_MCLK主时钟，至少是采样频率的256倍*/
#define I2S_RECV_BUF_SIZE       2400                    /*接收缓冲区大小*/
#define I2S_SAMPLE_RATE         44100                   /*采样率*/
#define I2S_MCLK_MULTIPLE_16BIT I2S_MCLK_MULTIPLE_256   /*主时钟倍率，如果不使用24位数据宽度，256足矣*/
#define I2S_MCLK_MULTIPLE_24BIT 192   /*128 * 3 = 384，必须是3的倍数，96000（采样率） × 384 = 36.864 MHz*/

extern i2s_chan_handle_t tx_handle;
extern i2s_chan_handle_t rx_handle;

esp_err_t myi2s_init(void);
void i2s_trx_start(void);
void i2s_trx_stop(void);
void i2s_deinit(void);
void i2s_set_samplerate_bits_sample(int samplerate, int bits_sample);
size_t i2s_tx_write(uint8_t* buffer, uint32_t frame_size);
size_t i2s_rx_read(uint8_t* buffer, uint32_t frame_size);
