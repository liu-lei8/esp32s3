#include "my_es8311.h"

i2c_obj_t es8311_i2c_master;
esp_codec_dev_handle_t my_codec_dev;


static const audio_codec_if_t* es8311_codec_if = NULL;
static const audio_codec_data_if_t *es8311_data_if = NULL;

uint8_t es8311_init(i2c_obj_t self)
{
    if (self.init_flag == ESP_OK)
    {
        es8311_i2c_master = self;
    }
    else
    {
        es8311_i2c_master = iic_init(I2C_NUM_0);
    }

    /*I2S音频数据传输配置*/
    audio_codec_i2s_cfg_t i2s_cfg = {
        .clk_src = I2S_CLK_SRC_DEFAULT,
        .port = I2S_NUM,
        .rx_handle = rx_handle,
        .tx_handle = tx_handle,
    };
    const audio_codec_data_if_t* data_if = audio_codec_new_i2s_data(&i2s_cfg);
    es8311_data_if = data_if;

    /*I2C ES8311寄存器设置*/
    audio_codec_i2c_cfg_t i2c_cfg = {
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .clock_speed_hz = IIC_FREQ,
        .port = es8311_i2c_master.port
    };
    const audio_codec_ctrl_if_t* out_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    const audio_codec_gpio_if_t* gpio_if = audio_codec_new_gpio();

    /*寄存器以及引脚配置*/
    es8311_codec_cfg_t es8311_cfg = {
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,  /*开启DAC和ADC*/
        .ctrl_if = out_ctrl_if,
        .gpio_if = gpio_if,
        .pa_pin = -1,               /*这里不使用，因为用xl9555来控制喇叭*/
        .use_mclk = true,
    };
    const audio_codec_if_t* out_codec_if = es8311_codec_new(&es8311_cfg);
    es8311_codec_if = out_codec_if;

    /*ES8311配置+数据接口配置*/
    esp_codec_dev_cfg_t dev_cfg = {
        .codec_if = out_codec_if,   /*es8311的配置接口*/
        .data_if = data_if,         /*i2s数据接口*/
        .dev_type = ESP_CODEC_DEV_TYPE_IN_OUT,     /*支持播放和录音*/
    };
    esp_codec_dev_handle_t codec_dev = esp_codec_dev_new(&dev_cfg);
    my_codec_dev = codec_dev;
    
    esp_codec_dev_set_out_vol(codec_dev, 0);        /*设置声音*/
    /*采样信息配置*/
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = I2S_SAMPLE_RATE,
        .channel = 2,                       /*双声道*/
        .bits_per_sample = I2S_DATA_BIT_WIDTH_16BIT,
        // .mclk_multiple = I2S_MCLK_MULTIPLE_16BIT,
    };
    return esp_codec_dev_open(codec_dev, &fs);          /*会自动使能I2S接收和传输*/
}

uint8_t es8311_write(uint8_t* data, int len)
{
    int aligned_len = len & ~3;     /* 向下取整到4的倍数 */
    if (aligned_len == 0) return 1;
    return esp_codec_dev_write(my_codec_dev, data, len);
}

uint8_t es8311_read(uint8_t* data, int len)
{
    esp_codec_dev_set_in_gain(my_codec_dev, 30);        /*设置麦克风增益*/
    return esp_codec_dev_read(my_codec_dev, data, len);
}

esp_err_t es8311_update_fs(int samplerate, int bits)
{
    int ret;

    if (es8311_codec_if == NULL || es8311_data_if == NULL) {
        return ESP_FAIL;
    }

    // 1. 先禁用数据接口（相当于暂停 I2S）
    // if (es8311_data_if->enable) {
    //     es8311_data_if->enable(es8311_data_if, ESP_CODEC_DEV_TYPE_IN_OUT, false);
    // }

    // 2. 配置 I2S 格式（关键：同时设置位宽和 mclk_multiple）
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = samplerate,
        .channel = 2,
        .bits_per_sample = bits,
        .mclk_multiple = I2S_MCLK_MULTIPLE_24BIT,       /*只能被重新设置一次*/
    };
    ret = es8311_data_if->set_fmt(es8311_data_if, ESP_CODEC_DEV_TYPE_IN_OUT, &fs); /*内部会禁用I2S，所以前面不用禁用*/
    if (ret != 0) {
        ESP_LOGE("ES8311", "I2S set_fmt failed");
        return ESP_FAIL;
    }

    // 3. 配置 ES8311 内部采样率和位宽
    ret = es8311_codec_if->set_fs(es8311_codec_if, &fs);
    if (ret != 0) {
        // 24bit + 384 可能因系数表缺失而返回错误，但数据路径通常已配置成功
        ESP_LOGW("ES8311", "codec set_fs returned %d", ret);
        // 不返回失败，继续执行
    }

    // 4. 重新启用数据接口
    if (es8311_data_if->enable) {
        es8311_data_if->enable(es8311_data_if, ESP_CODEC_DEV_TYPE_IN_OUT, true);
    }

    return ESP_OK;
}

