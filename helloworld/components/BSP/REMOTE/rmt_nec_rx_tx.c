#include "rmt_nec_rx_tx.h"

uint16_t s_nec_code_address = 0x0000;
uint16_t s_nec_code_command = 0x0000;

QueueHandle_t receive_queue = NULL;
rmt_channel_handle_t rx_channel = NULL;
rmt_channel_handle_t tx_channel = NULL;
rmt_symbol_word_t raw_symbols[64];
rmt_receive_config_t receive_config;
rmt_transmit_config_t transmit_config;

rmt_encoder_handle_t nec_encoder = NULL;
ir_nec_scan_code_t scan_code = {0};

static bool rmt_nec_rx_done_callback(rmt_channel_handle_t rx_chan, const rmt_rx_done_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_wakeup = pdFALSE;

    QueueHandle_t receive_queue = (QueueHandle_t)user_ctx;
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup); /*将收到的RMT数据通过消息队列发送到解析任务*/

    return high_task_wakeup == pdTRUE;
}

esp_err_t rmt_nec_rx_init(void)
{
    ESP_ERROR_CHECK(gpio_reset_pin(RMT_IN_GPIO_PIN));

    rmt_rx_channel_config_t rx_channel_cfg = {
        .gpio_num = RMT_IN_GPIO_PIN,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = 64,                /*通道能一次存储RMT符号的数量*/
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_channel_cfg, &rx_channel));

    /*配置红外接收完成回调*/
    receive_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));/*创建消息队列，用于接收红外编码*/
    assert(receive_queue);
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rmt_nec_rx_done_callback,
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_channel, &cbs, receive_queue));

    /*NEC时序要求*/
    receive_config.signal_range_min_ns = 1250; /* NEC信号的最短持续时间为560us，1250ns＜560us，小于1250ns将被视为噪声 */
    receive_config.signal_range_max_ns = 12000000; /*NEC信号最长持续时间为9000us，大于12000000ns被视为空闲状态，接收会提前停止并触发回调函数*/

    ESP_ERROR_CHECK(rmt_enable(rx_channel));        /*使能RMT通道*/
    ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config));                                /*准备接收*/

    return ESP_OK;
}

esp_err_t rmt_nec_tx_init(void)
{
    ESP_LOGI("rmt_tx", "Creat RMT TX channel");
    gpio_reset_pin(RMT_OUT_GPIO_PIN);

    rmt_tx_channel_config_t tx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .gpio_num = RMT_OUT_GPIO_PIN,
        .trans_queue_depth = 4,     /*允许在后台挂起的事务数，rmt_tx_wait_all_done该函数会等待所有事务传输完毕*/
        .mem_block_symbols = 64,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_channel_cfg, &tx_channel));  /*创建RMT发送通道*/

    /*配置载波与占空比*/
    ESP_LOGI("rmt_tx", "Modulate carrier to TX channel");
    rmt_carrier_config_t carrier_cfg = {
        .frequency_hz = 38000,      /*载波频率38KHz，0表示禁用载波*/
        .duty_cycle = 0.33          /*载波占空比,发送红外光的占空比一般为1/3*/
    };
    /*对发送通道应用载波调制功能*/
    ESP_ERROR_CHECK(rmt_apply_carrier(tx_channel, &carrier_cfg));

    transmit_config.loop_count = 0;    /*0为不循环，-1为无限循环发送*/

    /*配置NEC编码器*/
    ESP_LOGI("rmt_tx", "Install IR NEC encoder");
    ir_nec_encoder_config_t nec_encoder_cfg = {
        .resolution = RMT_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_ir_nec_encoder(&nec_encoder_cfg, &nec_encoder));

    ESP_ERROR_CHECK(rmt_enable(tx_channel));    /*使能发送通道*/
    ESP_ERROR_CHECK(rmt_transmit(tx_channel, nec_encoder, &scan_code, sizeof(scan_code), &transmit_config));    /*开始通过RMT通道传输数据*/

    return ESP_OK;
}

static inline bool rmt_nec_check_range(uint32_t signal_duration, uint32_t spec_duration)
{
    return (signal_duration < (spec_duration + RMT_NEC_DECODE_MARGIN)) && (signal_duration > (spec_duration - RMT_NEC_DECODE_MARGIN));
}

static bool rmt_nec_check_logic0(rmt_symbol_word_t* rmt_nec_symbols)
{
    return rmt_nec_check_range(rmt_nec_symbols->duration0, NEC_PAYLOAD_ZERO_DURATION_0) && rmt_nec_check_range(rmt_nec_symbols->duration1, NEC_PAYLOAD_ZERO_DURATION_1);
}

static bool rmt_nec_check_logic1(rmt_symbol_word_t* rmt_nec_symbols)
{
    return rmt_nec_check_range(rmt_nec_symbols->duration0, NEC_PAYLOAD_ONE_DURATION_0) && rmt_nec_check_range(rmt_nec_symbols->duration1, NEC_PAYLOAD_ONE_DURATION_1);
}

static bool rmt_nec_parse_frame(rmt_symbol_word_t* rmt_nec_symbols)
{
    rmt_symbol_word_t* cur = rmt_nec_symbols;
    uint16_t address = 0;
    uint16_t command = 0;

    bool valid_leading_code = rmt_nec_check_range(cur->duration0, NEC_LEADING_CODE_DURATION_0) && rmt_nec_check_range(cur->duration1, NEC_LEADING_CODE_DURATION_1);

    if (!valid_leading_code)
    {
        return false;
    }

    cur++;
    for (uint8_t i = 0; i < 16; i++)        /*给address变量写入地址+地址反码*/
    {
        if (rmt_nec_check_logic0(cur))
        {
            address &= ~(1 << i);
        }
        else if(rmt_nec_check_logic1(cur))
        {
            address |= 1 << i;
        }
        else
        {
            return false;
        }
        cur++;
    }
    
    for (uint8_t i = 0; i < 16; i++)        /*给command变量写入命令+命令反码*/
    {
        if (rmt_nec_check_logic0(cur))
        {
            command &= ~(1 << i);
        }
        else if (rmt_nec_check_logic1(cur))
        {
            command |= 1 << i;
        }
        else
        {
            return false;
        }
        cur++;
    }
    
    /*保存数据地址和命令，用于判断重复按键*/
    s_nec_code_address = address;
    s_nec_code_command = command;

    return true;

}

static bool rmt_nec_check_repeat(rmt_symbol_word_t* rmt_nec_symbols)
{
    return rmt_nec_check_range(rmt_nec_symbols->duration0, NEC_REPEAT_CODE_DURATION_0) && rmt_nec_check_range(rmt_nec_symbols->duration1, NEC_REPEAT_CODE_DURATION_1);
}

void rmt_rx_scan(rmt_symbol_word_t* rmt_nec_symbols, size_t num_symbols)
{
    static uint8_t rmt_data = 0;
    uint8_t tbuf[40];
    char* str = NULL;

    ESP_LOGI("rmt_rx", "Reception Device Address: %04X", s_nec_code_address);

    switch (num_symbols)
    {
        case 34:        /*正常NEC数据帧*/
        {
            if (rmt_nec_parse_frame(rmt_nec_symbols))
            {
                rmt_data = s_nec_code_command & 0xFF;       /*得到命令码，对应遥控器按键值*/
                switch (rmt_data)
                {
                case 0x45: str = "1"; break;
                case 0x46: str = "2"; break;
                case 0x47: str = "3"; break;
                case 0x44: str = "4"; break;
                case 0x40: str = "5"; break;
                case 0x43: str = "6"; break;
                case 0x07: str = "7"; break;
                case 0x15: str = "8"; break;
                case 0x09: str = "9"; break;
                case 0x16: str = "*"; break;
                case 0x19: str = "0"; break;
                case 0x0D: str = "#"; break;
                case 0x18: str = "UP"; break;
                case 0x52: str = "DOWN"; break;
                case 0x08: str = "LEFT"; break;
                case 0x5A: str = "RIGHT"; break;
                case 0x1C: str = "OK"; break;
                default:
                    break;
                }

                lcd_fill(86, 126, 140, 176, WHITE);
                sprintf((char*)tbuf, "%#04X", rmt_data);
                lcd_show_string(86, 140, (char*)tbuf, BLUE, WHITE, 16, 0);
                lcd_show_string(86, 160, str, BLUE, WHITE, 16, 0);

                ESP_LOGI("rmt_rx", "KEYVAL: %d, Command: %04X", rmt_data, s_nec_code_command);
            }
            break;
        }
        case 2:         /*重复数据帧*/
        {
            if (rmt_nec_check_repeat(rmt_nec_symbols))
            {
                ESP_LOGI("rmt_rx", "KEYVAL: %d, Command:%04X, repeat", rmt_data, s_nec_code_command);
            }
            break;
        }
        default: ESP_LOGI("rmt_rx", "Unknown NEC frame"); break;
    }

}

void rmt_rx_scan1(rmt_symbol_word_t* rmt_nec_symbols, size_t num_symbols)
{
    uint8_t tbuf[10];

    switch (num_symbols)
    {
        case 34:
        {
            if (rmt_nec_parse_frame(rmt_nec_symbols))
            {
                lcd_fill(110, 134, 160, 176, WHITE);
                sprintf((char*)tbuf, "%d", s_nec_code_command);
                printf("RX KEYCNT = %d\n", s_nec_code_command);
                lcd_show_string(110, 160, (char*)tbuf, BLUE, WHITE, 16, 1);
            }
            break;
        }
        case 2:
        {
            if (rmt_nec_check_repeat(rmt_nec_symbols))
            {
                printf("RX KEYCNT: %d, repeat\n", s_nec_code_command);
            }
            break;
        }
        default: printf("Unknown NEC frame\n\n"); break;
    }
}
