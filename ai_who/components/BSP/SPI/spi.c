#include "spi.h"

extern spi_transaction_t pending_trans[MAX_PENDING_TRANS];
extern int pending_head;
extern int pending_tail;
static uint8_t buf_static[20] = {0};
static uint8_t buf_count = 0;

void spi2_init(void)
{
    esp_err_t ret = 0;
    
    /*  ESP32-S3 的 SPI 硬件限制：单次 SPI 事务长度不能超过 SPI_LL_DATA_MAX_BIT_LEN，对于 ESP32-S3 该值为 1<<18 = 262144 位，即 32768 字节。*/
    spi_bus_config_t spi_bus_cfg = {
        .miso_io_num = SPI_MISO_GPIO_PIN,
        .mosi_io_num = SPI_MOSI_GPIO_PIN,
        .sclk_io_num = SPI_CLK_GPIO_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        // .max_transfer_sz = SPI_MAX_DMA_LEN,        /*可能spi的DMA单次最大只支持4092*/
        .max_transfer_sz = 32768,
    };

    ret = spi_bus_initialize(SPI2_HOST, &spi_bus_cfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
}

void spi2_write_cmd(spi_device_handle_t handle, uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t = {0};

    t.length = 8;
    t.tx_buffer = &cmd;
    ret = spi_device_polling_transmit(handle, &t);
    ESP_ERROR_CHECK(ret);
}

void spi2_write_data(spi_device_handle_t handle, const uint8_t* data, int len)
{
    esp_err_t ret;
    spi_transaction_t t = {0};

    if (len == 0)
    {
        return;
    }

    t.tx_buffer = data;
    t.length = len * 8;
    ret = spi_device_polling_transmit(handle, &t);
    ESP_ERROR_CHECK(ret);
}

uint8_t spi2_transfer_byte(spi_device_handle_t handle, uint8_t data)
{
    spi_transaction_t t;

    memset(&t, 0, sizeof(t));

    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.length = 8;
    t.tx_data[0] = data;
    spi_device_polling_transmit(handle, &t);        /*中断传输*/

    return t.rx_data[0];
}

/* 下面两个尝试使用异步传输，最终失败了，可能ESP-IDF 的 SPI 主机驱动内部对描述符池、轮询/队列混用、事务完成事件的管理比你看到的 API 要复杂*/
void spi2_write_cmd1(spi_device_handle_t handle, uint8_t cmd)
{
    if (buf_count >= 20) buf_count = 0;
    buf_static[buf_count] = cmd;
    spi_transaction_t *t = &pending_trans[pending_head % MAX_PENDING_TRANS];
    memset(t, 0, sizeof(spi_transaction_t));
    t->length = 8;
    t->tx_buffer = buf_static + buf_count;
    spi_device_queue_trans(handle, t, portMAX_DELAY);
    pending_head++;
    spi_transaction_t* rtrans;
    spi_device_get_trans_result(handle, &rtrans, portMAX_DELAY);
    pending_tail++;
    
    buf_count++;
}

void spi2_write_data1(spi_device_handle_t handle, const uint8_t* data, int len)
{
    if (buf_count >= 20) buf_count = 0;
    uint32_t i = 0;
    for (i = 0; i < len; i++)
    {
        buf_static[buf_count + i] = data[i];
    }
    
    spi_transaction_t *t = &pending_trans[pending_head % MAX_PENDING_TRANS];
    memset(t, 0, sizeof(spi_transaction_t));
    t->length = len * 8;
    t->tx_buffer = buf_static + buf_count;
    spi_device_queue_trans(handle, t, portMAX_DELAY);
    pending_head++;
    spi_transaction_t* rtrans;
    spi_device_get_trans_result(handle, &rtrans, portMAX_DELAY);
    pending_tail++;

    buf_count += i;
}
