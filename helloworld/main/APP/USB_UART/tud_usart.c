#include "tud_usart.h"

static const char* TAG = "usb_cdc";
static uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE + 1];

/**
 * @brief       USB虚拟串口接收回调函数
 * @param       itf     :  CDC端口
 * @param       event   :  CDC事件
 * @retval      无
 */
void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t* event)
{
    size_t rx_size = 0;
    uint8_t temp[1280];
    

    esp_err_t ret = tinyusb_cdcacm_read(itf, buf, CFG_TUD_CDC_RX_BUFSIZE, &rx_size);    /* 读取串口数据 */
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Data from channel: %d", itf);
        ESP_LOG_BUFFER_HEXDUMP(TAG, buf, rx_size, ESP_LOG_INFO);  /* 将缓冲区里二进制的值转换为16进制打印到日志中查看 */
    }
    else ESP_LOGE(TAG, "Read error");

    sprintf((char*)temp, "The message: %s", buf);                   /* temp缓冲区必须必buf大才行 */
    rx_size = strlen((const char*)temp);
    tinyusb_cdcacm_write_queue(itf, temp, rx_size);                                      /* 回发数据 */
    tinyusb_cdcacm_write_flush(itf, 0);
}

/**
 * @brief       判断接线状态的回调函数，主要判断上位机串口是否已经连接上了
 * @param       itf     :  CDC端口
 * @param       event   :  CDC事件
 * @retval      无
 */
void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t* event)
{
    /* Data Terminal Ready数据终端就绪，一般是电脑发给板子，告诉板子“我准备好了，可以开始通信了”有时也用来触发板子复位*/
    int dtr = event->line_state_changed_data.dtr;
    /* Request To Send请求发送，主要用来做流控制的，防止数据发送太快对方处理不过来 */
    int rts = event->line_state_changed_data.rts;
    ESP_LOGI(TAG, "Line state changed on cahnnel: %d, DTR: %d, RTS: %d", itf, dtr, rts);
}

/**
 * @brief       USB虚拟串口初始化
 * @param       无
 * @retval      无
 */
void tud_usb_usart(void)
{
    ESP_LOGI(TAG, "USB initialization");

    /* 版本2.x 使用 TINYUSB_DEFAULT_CONFIG() 宏创建默认配置
     * 默认配置已自动处理描述符、端口选择、PHY、任务参数等 */
    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    /* usb设备登记 */
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    /* 2.x 的 CDC ACM 配置结构体，已移除 .usb_dev和.rx_unread_buf_sz字段 */
    tinyusb_config_cdcacm_t acm_cfg = {
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = &tinyusb_cdc_rx_callback,
        .callback_rx_wanted_char = NULL,
        .callback_line_coding_changed = NULL,
        .callback_line_state_changed = NULL,
    };
    /* usb cdc初始化 */
    ESP_ERROR_CHECK(tinyusb_cdcacm_init(&acm_cfg));

    /* 注册回调函数 */
    ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(TINYUSB_CDC_ACM_0, CDC_EVENT_LINE_STATE_CHANGED, &tinyusb_cdc_line_state_changed_callback));

    ESP_LOGI(TAG, "USB initialization DONE");
}
