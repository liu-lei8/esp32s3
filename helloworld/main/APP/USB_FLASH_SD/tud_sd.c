#include "tud_sd.h"

extern sdmmc_card_t* card;
static const char* TAG = "tud_sd";

void tud_usb_sd(void)
{
    /* 安装msc驱动 */
    const tinyusb_msc_driver_config_t msc_driver_cfg = {
        .callback = NULL,
        .callback_arg = NULL,
    };
    ESP_ERROR_CHECK(tinyusb_msc_install_driver(&msc_driver_cfg));

    /* 创建SDMMC存储实例 */
    tinyusb_msc_storage_handle_t storage_hdl = NULL;
    const tinyusb_msc_storage_config_t storage_cfg = {
        .mount_point = TINYUSB_MSC_STORAGE_MOUNT_APP,           /* 初始挂载给APP */
        .medium.card = card,
        .fat_fs = {
            .base_path = BASE_PATH,
            .config = {
                .max_files = 5,
                .format_if_mount_failed = false,
                .allocation_unit_size = 16 * 1024,              // 增大簇大小，减少FAT表碎片
            },
        },
    };
    ESP_ERROR_CHECK(tinyusb_msc_new_storage_sdmmc(&storage_cfg, &storage_hdl));

    /* 安装USB驱动 */
    const tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
}

