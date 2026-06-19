#pragma once /*spi_sd.h*/

#include "driver/sdspi_host.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "spi.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"

#define SD_NUM_CS       GPIO_NUM_2
#define MOUNT_POINT     "/0:"

/*该初始化直接手动配置的参数*/
esp_err_t sd_spi_init(void);
/*该初始化使用的宏配置参数*/
esp_err_t sd_spi_init_test(void);
/**
 * @brief 获取MicroSD卡的总内存和空闲内存的KB数
 * @param out_total_bytes:要获取的总内存
 * @param out_free_bytes:要获取的空闲内存
 */
void sd_get_fatfs_usage(size_t* out_total_bytes, size_t* out_free_bytes);