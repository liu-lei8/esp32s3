#include "spi_sd.h"

spi_device_handle_t my_sd_handle;
static const char *TAG = "sd_spi";
const char mount_point[] = MOUNT_POINT;
sdmmc_card_t* card;

esp_err_t sd_spi_init(void)
{
    esp_err_t ret = ESP_OK;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 20 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = SD_NUM_CS,
        .queue_size = 7,
    };
    /*添加spi总线设备*/
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &my_sd_handle);

    /*文件系统挂载配置*/
    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        /*如果挂载失败：true-会重新分区和格式化，false-则不会*/
        .format_if_mount_failed = false,
        .max_files = 5,                         /*打开文件的最大数量*/
        .allocation_unit_size = 16 * 1024       /*硬盘分区簇的大小16KB*/
    };

    /*SD卡参数的配置*/
    sdmmc_host_t host = {
        /*定义主机属性标志：SPI协议并且可以调用deinit函数*/
        .flags = SDMMC_HOST_FLAG_SPI | SDMMC_HOST_FLAG_DEINIT_ARG,
        .slot = SPI2_HOST,                              /*使用SPI2端口*/
        .max_freq_khz = SDMMC_FREQ_DEFAULT,             /*主机支持的最大频率：20000khz*/
        .io_voltage = 3.3f,                             /*控制器使用的i/o电压*/
        .init = &sdspi_host_init,                /*用于初始化驱动程序的主机函数*/
        .set_bus_width = NULL,                          /*用于设置总线宽度的主机函数*/
        .get_bus_width = NULL,                          /*获取总线宽度的主机函数*/
        .set_bus_ddr_mode = NULL,                       /*设置DDR模式的主机函数*/
        .set_card_clk = &sdspi_host_set_card_clk,       /*设置板卡时钟频率的主机函数*/
        .do_transaction = &sdspi_host_do_transaction,   /*执行事务的主机函数*/
        .deinit_p = &sdspi_host_remove_device,          /*用于取消初始化驱动程序的主机函数*/
        .io_int_enable = &sdspi_host_io_int_enable,     /*启用SDIO中断的主机函数*/
        .io_int_wait = &sdspi_host_io_int_wait,         /*等待SDIO中断线路激活的主机函数*/
        .command_timeout_ms = 0,                /*0为默认系统超时时间，即等待SD卡响应时间*/
        .get_real_freq = &sdspi_host_get_real_freq,     /*这里正点原子教程漏了导致一直出错*/
    };

    /*SD卡引脚配置*/
    sdspi_device_config_t slot_cfg = {
        .gpio_cd = GPIO_NUM_NC,
        .gpio_cs = SD_NUM_CS,
        .gpio_int = GPIO_NUM_NC,
        .gpio_wp = GPIO_NUM_NC,
        .host_id = host.slot,
    };

    /*挂载文件系统*/
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_cfg, &mount_cfg, &card);

    if (ret != ESP_OK)
    {
        spi_bus_remove_device(my_sd_handle);    /*移除SPI总线上的SD卡*/
    }

    return ret;
}

esp_err_t sd_spi_init_test(void)
{
    esp_err_t ret = ESP_OK;

    spi_device_interface_config_t devcfg = {
    .clock_speed_hz = 20 * 1000 * 1000,
    .mode = 0,
    .spics_io_num = SD_NUM_CS,
    .queue_size = 7,
    };
    /*添加spi总线设备*/
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &my_sd_handle));


    /* 文件系统挂载配置 */
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,                            /* 如果挂载失败：true会重新分区和格式化/false不会重新分区和格式化 */
        .max_files = 5,                                             /* 打开文件最大数量 */
        .allocation_unit_size = 4 * 1024 * sizeof(uint8_t)          /* 硬盘分区簇的大小 */
    };

    /* SD卡参数配置 */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    /* SD卡引脚配置 */
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id   = host.slot;
    slot_config.gpio_cs   = SD_NUM_CS;
    slot_config.gpio_cd   = GPIO_NUM_NC;
    slot_config.gpio_wp   = GPIO_NUM_NC;
    slot_config.gpio_int  = GPIO_NUM_NC;

    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);      /* 挂载文件系统 */
    if (ret)
    {
        esp_vfs_fat_sdcard_unmount(mount_point, card);
        spi_bus_remove_device(my_sd_handle);    /*移除SPI总线上的SD卡*/
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    return ret;
}

void sd_get_fatfs_usage(size_t* out_total_bytes, size_t* out_free_bytes)
{
    FATFS* fs;
    size_t free_clusters;
    int res = f_getfree("0:", (DWORD*)&free_clusters, &fs);
    assert(res == FR_OK);
    
    size_t total_sectors = (fs->n_fatent - 2) * fs->csize;  /*总扇区数量*/
    size_t free_sectors = free_clusters * fs->csize;        /*空闲扇区数量*/

    size_t sd_total = total_sectors / 1024;     /*先除1024是为了怕超过size_t类型最大值大小*/
    size_t sd_total_KB = sd_total * fs->ssize;             /*多少KB*/
    size_t sd_free = free_sectors / 1024;
    size_t sd_free_KB = sd_free * fs->ssize;

    if (out_total_bytes != NULL)
    {
        *out_total_bytes = sd_total_KB;
    }
    if (out_free_bytes != NULL)
    {
        *out_free_bytes = sd_free_KB;
    }
}

