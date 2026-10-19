#include "bsp/xl9555.h"
#include "bsp/atk_dnesp32s3m.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "XL9555";
static i2c_master_dev_handle_t s_xl_dev = NULL;
static uint16_t s_out_cache = 0;
static bool s_init = false;

static esp_err_t xl_write_reg(uint8_t reg, const uint8_t *data, size_t len)
{
    uint8_t buf[3] = { reg, 0, 0 };
    for (size_t i = 0; i < len && i < 2; i++) buf[i + 1] = data[i];
    return i2c_master_transmit(s_xl_dev, buf, len + 1, 1000);
}

esp_err_t xl9555_pin_write_bsp(uint16_t pin, bool level)
{
    if (!s_init) return ESP_ERR_INVALID_STATE;

    if (level) s_out_cache |= pin;
    else       s_out_cache &= ~pin;

    uint8_t data[2] = { (uint8_t)(s_out_cache & 0xFF), (uint8_t)((s_out_cache >> 8) & 0xFF) };
    return xl_write_reg(XL9555_OUTPUT_PORT0_REG, data, 2);
}

esp_err_t xl9555_init_bsp(void)
{
    if (s_init) return ESP_OK;

    i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
    if (!bus) return ESP_FAIL;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = XL9555_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &s_xl_dev), TAG, "add dev failed");

    // 配置方向：P04(PWDN), P05(RST), P12(LCD_RST), P13(LCD_PWR) 为输出
    uint16_t ioconfig = 0xF3CF;
    uint8_t cfg[2] = { (uint8_t)(ioconfig & 0xFF), (uint8_t)((ioconfig >> 8) & 0xFF) };
    ESP_RETURN_ON_ERROR(xl_write_reg(XL9555_CONFIG_PORT0_REG, cfg, 2), TAG, "config failed");

    s_out_cache = 0xF7EF;
    uint8_t init[2] = { (uint8_t)(s_out_cache & 0xFF), (uint8_t)((s_out_cache >> 8) & 0xFF) };
    ESP_RETURN_ON_ERROR(xl_write_reg(XL9555_OUTPUT_PORT0_REG, init, 2), TAG, "init output failed");

    s_init = true;
    ESP_LOGI(TAG, "XL9555 initialized, out_cache=0x%04X", s_out_cache);
    return ESP_OK;
}