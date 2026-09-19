#include "tud_flash.h"

static const char* TAG = "tud_flash";


/**
 * @brief 子分区初始化
 * @param wl_handle: wear leavelling handle
 * @retval ESP_OK:初始化成功
 */
static esp_err_t storage_init_spiflash(wl_handle_t* wl_handle)
{
    ESP_LOGI(TAG, "Initializing wear levelling");

    const esp_partition_t* data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "storage");        /* 查找storage分区 */

    if (data_partition == NULL)
    {
        ESP_LOGE(TAG, "Failed to find FATFS partition. Check the partition table.");
        return ESP_ERR_NOT_FOUND;
    }

    return wl_mount(data_partition, wl_handle);         /* 挂载磨损均衡层 */
}

void tud_usb_flash(void)
{
     // 1. 初始化磨损均衡
    static wl_handle_t wl_hdl = WL_INVALID_HANDLE;
    ESP_ERROR_CHECK(storage_init_spiflash(&wl_hdl));     /* 初始化Flash存储 */

    // 2. 先安装 MSC 驱动
    const tinyusb_msc_driver_config_t msc_driver_cfg = {
        .callback = NULL,
        .callback_arg = NULL,
    };
    ESP_ERROR_CHECK(tinyusb_msc_install_driver(&msc_driver_cfg));

    /* 3. MSC 存储配置 — 挂载点和 FATFS 参数都在这里指定 */
    tinyusb_msc_storage_handle_t storage_hdl = NULL;
    const tinyusb_msc_storage_config_t storage_cfg = {
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_APP,   /* 初始挂载到 APP (本地访问) */
        .medium.wl_handle = wl_hdl,                     /* 介质为 SPI Flash 磨损均衡 */
        .fat_fs = {
            .base_path = BASE_POINT,                    /* 本地访问路径 /1: */
            .config = {
                .max_files = 5,                         /* 最多同时打开5个文件 */
                .format_if_mount_failed = false,        /* 挂载失败不自动格式化 */
            },
        },
    };
    ESP_ERROR_CHECK(tinyusb_msc_new_storage_spiflash(&storage_cfg, &storage_hdl));

    /* 4. USB配置 */
    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
}
