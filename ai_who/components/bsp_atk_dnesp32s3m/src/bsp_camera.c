#include "bsp/atk_dnesp32s3m.h"
#include "bsp/xl9555.h"
#include "esp_log.h"
#include "esp_check.h"

esp_err_t bsp_camera_start(const bsp_camera_cfg_t *cfg)
{
    // 只负责初始化 BSP 的 I2C 和 IO 扩展芯片
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), "BSP_CAM", "i2c init failed");
    ESP_RETURN_ON_ERROR(xl9555_init_bsp(), "BSP_CAM", "xl9555 init failed");
    
    return ESP_OK;
}