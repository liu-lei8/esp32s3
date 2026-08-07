#include "myi2s.h"

i2s_chan_handle_t tx_handle = NULL;     /*I2S发送通道句柄*/
i2s_chan_handle_t rx_handle = NULL;     /*I2S接收通道句柄*/
i2s_std_config_t my_std_cfg;            /*标准模式配置结构体，做为全局变量便于重新设置位宽和采样率*/

esp_err_t myi2s_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);

    chan_cfg.auto_clear = true;     /*自动清除DMA缓冲区里遗留的数据*/
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle));

    /*标准通信模式配置*/
    i2s_std_config_t std_cfg = {
        .clk_cfg = {    /*时钟配置 可用I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE)宏函数辅助配置*/
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .sample_rate_hz = I2S_SAMPLE_RATE,
            .mclk_multiple = I2S_MCLK_MULTIPLE_16BIT,
        },

        .slot_cfg = {   /*声道配置 可用I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)宏函数辅助配置(支持16位宽采样数据)*/
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,     /*声道支持16位宽的采样数据*/
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,      /*通道位宽,24位数据时，槽宽一般设置为32位*/
            .slot_mode = I2S_SLOT_MODE_STEREO,              /*立体声模式*/
            .slot_mask = I2S_STD_SLOT_BOTH,                 /*启用左右声道*/
            .ws_width = I2S_DATA_BIT_WIDTH_16BIT,           /*字选择位宽*/
            .ws_pol = false,                                /*字选择极性，false标准模式低电平左声道，高电平右声道*/
            .bit_shift = true,                              /*位移位(Philips模式下配置，Philips 模式需要数据延迟一个 BCLK)*/
            .left_align = false,                            /*不使用左对齐模式，使用标准I2S（Philips标准）*/
            .big_endian = false,                            /*小端模式*/
            .bit_order_lsb = false,                         /*MSB高位先行*/
        },

        .gpio_cfg = {                   /*引脚配置*/
            .mclk = I2S_MCK_IO,         /*主时钟*/
            .bclk = I2S_BCK_IO,         /*位时钟*/
            .ws = I2S_WS_IO,            /*字(声道)选择线*/
            .dout = I2S_DO_IO,          /*串行数据输出线*/
            .din = I2S_DI_IO,           /*串行数据输入线*/
            .invert_flags = {           /*信号极性不翻转*/
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,    
            }
        }
    };

    my_std_cfg = std_cfg;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));    /*初始化TX通道*/
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));    /*初始化RX通道*/
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));                     /*启用TX通道*/
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));                     /*启用RX通道*/

    return ESP_OK;
}

void i2s_trx_start(void)
{
    esp_err_t ret;
    ret = i2s_channel_enable(tx_handle);
    /* 可能出现连续两次开启的无效状态，那么包容无效状态情况 */
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(ret);
    }
    
    ret = i2s_channel_enable(rx_handle);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(ret);
    }
}

void i2s_trx_stop(void)
{
    esp_err_t ret;
    ret = i2s_channel_disable(tx_handle);
    /* 因为在music任务里和i2s_set_samplerate_bits_sample()函数中两次暂停会出现无效状态，那么包容无效状态情况 */
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(ret);
    }
    
    ret = i2s_channel_disable(rx_handle);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(ret);
    }
}

void i2s_deinit(void)
{
    ESP_ERROR_CHECK(i2s_del_channel(tx_handle));
    ESP_ERROR_CHECK(i2s_del_channel(rx_handle));
}

/**
 * @brief 重新设置采样率和位宽
 * @note 这函数不用了，在es8311_update_fs里已经都设置了
 * @param samplerate:采样率
 * @param bits_sample:位宽
 */
void i2s_set_samplerate_bits_sample(int samplerate, int bits_sample)
{
    i2s_trx_stop(); /*如果需要更新声道和时钟配置，需要在更新前禁用通道*/

    my_std_cfg.slot_cfg.ws_width = bits_sample;         /*设置字选择位宽*/
    my_std_cfg.slot_cfg.data_bit_width = bits_sample;   /*设置数据位宽*/

    if (bits_sample == I2S_DATA_BIT_WIDTH_24BIT)
    {
        my_std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;  /*24位数据需要用32位槽宽*/
        my_std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_24BIT;     /*设置主时钟倍率*/
    }
    else 
    {
        my_std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
        my_std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_24BIT;     /*有24位宽的WAV文件必须设为192或384*/
    }

    my_std_cfg.clk_cfg.sample_rate_hz = samplerate;     /*设置采样率*/

    ESP_ERROR_CHECK(i2s_channel_reconfig_std_slot(tx_handle, &my_std_cfg.slot_cfg));
    ESP_ERROR_CHECK(i2s_channel_reconfig_std_clock(tx_handle, &my_std_cfg.clk_cfg));
    ESP_ERROR_CHECK(i2s_channel_reconfig_std_slot(rx_handle, &my_std_cfg.slot_cfg));
    ESP_ERROR_CHECK(i2s_channel_reconfig_std_clock(rx_handle, &my_std_cfg.clk_cfg));
}

/**
 * @brief I2S传输数据
 * @param buffer：数据存储区的首地址
 * @param frame_size：缓冲区大小
 * @return 实际发送的数据长度（字节）
 */
size_t i2s_tx_write(uint8_t* buffer, uint32_t frame_size)
{
    size_t bytes_written;
    ESP_ERROR_CHECK(i2s_channel_write(tx_handle, buffer, frame_size, &bytes_written, 1000));
    return bytes_written;
}

/**
 * @brief I2S接收数据
 * @param buffer：数据存储区的首地址
 * @param frame_size：缓冲区大小
 * @return 实际接收的数据长度（字节）
 */
size_t i2s_rx_read(uint8_t* buffer, uint32_t frame_size)
{
    size_t bytes_read;
    ESP_ERROR_CHECK(i2s_channel_read(rx_handle, buffer, frame_size, &bytes_read, 1000));
    return bytes_read;
}
