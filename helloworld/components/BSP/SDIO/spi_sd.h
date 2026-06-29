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

/**
 * @brief 获取path路径下的目标图片文件的数量
 * @param path 路径信息
 * @return 返回图片文件的数量
 */
uint16_t pic_get_tnum(char* path);

/**
 * @brief 将摄像头的jpeg格式的数据存储到该路径中"0:/PICTURE"
 * @param pvParameters: 未用到该参数
 */
void task3(void* pvParameters);