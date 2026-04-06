#include "spi.h"

void spi2_init(void)
{
    esp_err_t ret = 0;
    
    spi_bus_config_t spi_bus_cfg = {
        .miso_io_num = SPI_MISO_GPIO_PIN,
        .mosi_io_num = SPI_MOSI_GPIO_PIN,
        .sclk_io_num = SPI_CLK_GPIO_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_MAX_DMA_LEN,        /*可能spi的DMA单次最大只支持4092*/
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
    ret = spi_device_polling_transmit(handle, &t);      /*轮询传输*/
    ESP_ERROR_CHECK(ret);
}

void spi2_write_data(spi_device_handle_t hanlde, const uint8_t* data, int len)
{
    esp_err_t ret;
    spi_transaction_t t = {0};

    if (len == 0)
    {
        return;
    }

    t.tx_buffer = data;
    t.length = len * 8;
    ret = spi_device_polling_transmit(hanlde, &t);
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