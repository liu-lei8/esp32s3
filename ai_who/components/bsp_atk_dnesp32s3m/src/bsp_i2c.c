#include "bsp/atk_dnesp32s3m.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "BSP_I2C";
static i2c_master_bus_handle_t s_i2c_bus = NULL;
static bool s_i2c_initialized = false;

esp_err_t bsp_i2c_init(void)
{
    if (s_i2c_initialized) return ESP_OK;

    const i2c_master_bus_config_t i2c_cfg = {
        .i2c_port = BSP_I2C_NUM,
        .sda_io_num = BSP_I2C_SDA,
        .scl_io_num = BSP_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_cfg, &s_i2c_bus), TAG, "i2c init failed");
    s_i2c_initialized = true;
    ESP_LOGI(TAG, "I2C initialized SDA=%d SCL=%d", BSP_I2C_SDA, BSP_I2C_SCL);
    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
    if (!s_i2c_initialized) return ESP_OK;
    i2c_del_master_bus(s_i2c_bus);
    s_i2c_bus = NULL;
    s_i2c_initialized = false;
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void)
{
    if (!s_i2c_initialized) {
        bsp_i2c_init();
    }
    return s_i2c_bus;
}