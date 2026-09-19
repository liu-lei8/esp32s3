#pragma once /*spi_sd.h*/

#include "driver/sdspi_host.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "spi.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "exfuns.h"
#include "camera.h"


#define SD_NUM_CS       GPIO_NUM_2
#define MOUNT_POINT     "/0:"

#define TASK3_STK_SIZE  5 * 1024        /*堆栈大小*/
#define TASK3_PRIORITY  4               /*任务优先级*/

extern SemaphoreHandle_t BinarySemaphore;
extern uint8_t sd_check_en;
extern TaskHandle_t task3_handler;
extern sdmmc_card_t* card;

/* 只初始化SD卡不挂载，在SD卡模拟U盘中用到，因为挂载会和msc配置冲突（msc内部会自动挂载） */
esp_err_t sd_spi_init(void);
/*该初始化直接手动配置的参数,初始化SD卡并挂载FATFS文件系统 */
esp_err_t sd_spi_mount_init(void);
/*该初始化使用的宏配置参数*/
esp_err_t sd_spi_mount_init_test(void);
/**
 * @brief 获取MicroSD卡的总内存和空闲内存的KB数
 * @param out_total_bytes:要获取的总内存
 * @param out_free_bytes:要获取的空闲内存
 */
void sd_get_fatfs_usage(size_t* out_total_bytes, size_t* out_free_bytes);

/**
 * @brief 获取path路径下的目标图片文件的数量
 *  @note 文件夹只能包含图片文件和.db类型的文件,才能得出图片文件的数量
 * @param path 路径信息
 * @return 返回图片文件的数量
 */
uint16_t pic_get_tnum(char* path);

/**
 * @brief 将摄像头的jpeg格式的数据存储到该路径中"0:/PICTURE"
 * @param pvParameters: 未用到该参数
 */
void task3(void* pvParameters);

/**
 * @brief 在指定目录里进行偏移找到对应的目录项
 *  @note 该函数修改了底层的dp->dptr(数据区起始偏移的字节量)，dp->sect(所在扇区)，
 *                         dp->dir(指向某个目录项的开头)
 * @param dp：指向目录对象的指针
 * @param ofs：目录表的偏移量，必须是32字节的倍数
 * @return FR_OK(0):成功，其他：失败。
 */
FRESULT atk_dir_sdi(FF_DIR* dp, DWORD ofs);