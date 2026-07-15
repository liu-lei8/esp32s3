#pragma once /*fonts.h 有关字库代码*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd.h"
#include "ff.h"
#include "esp_partition.h"
//#include "spi_flash_mmap.h"

/*字库区域占总扇区数的大小（3个字库+unigbk表+字库信息=3238700字节，约占791个扇区，一个扇区4K字节）*/
#define FONTSECSIZE     791
/*单个扇区的字节数，每次操作限制在4K内*/
#define SECTOR_SIZE     0x1000

/* 分区表字库存放的起始地址
 * 字库信息保存的首地址，占33个字节，第1个字节用于标记字库是否存在。
 * 后续每8个字节一组，分别保存不同大小字体的起始地址和文件大小。
 */
#define FONTINFOADDR    0

/**字库信息结构体定义
 * 用来保存字库的基本信息，地址和大小等
*/
typedef struct {
    uint8_t fontok;
    uint32_t ugbkaddr;
    uint32_t ugbksize;
    uint32_t f12addr;
    uint32_t gbk12size;
    uint32_t f16addr;
    uint32_t gbk16size;
    uint32_t f24addr;
    uint32_t gbk24size;
}_font_info;

extern _font_info ftinfo;
extern uint8_t *unigbk_table;

/**
 * @brief 分区表读取数据
 * @param offset 读取数据的起始地址
 * @param buffer 读取数据的缓冲区
 * @param lenght:要读取数据的字节数
 * @return ESP_OK:成功，其他失败
 */
esp_err_t fonts_partition_read(uint32_t offset, void* buffer, uint32_t lenght);

/**
 * @brief 分区表写入数据
 * @param offset 写入数据的起始地址
 * @param buffer 写入数据的存储区
 * @param lenght:要写入数据的字节数
 * @return ESP_OK:成功，其他失败
 */
esp_err_t fonts_partition_write(uint32_t offset, void* buffer, uint32_t lenght);

/**
 * @brief 擦除某个扇区
 * @param offset 擦除的起始地址
 * @return ESP_OK:成功，其他失败
 */
esp_err_t fonts_partition_erase_sector(uint32_t offset);

/**
 * @brief 在lcd上显示当前字库更新进度，显示更新百分比
 * @param xy:lcd显示坐标
 * @param size:lcd显示字体大小
 * @param totsize:整个文件的大小
 * @param pos:当前文件指针位置，即已经更新的字节数
 * @param fc:lcd字体颜色
 * @param bc:lcd字体背景颜色
 */
void fonts_progress_show(uint16_t x, uint16_t y, uint8_t size, uint32_t totsize, uint32_t pos, uint16_t fc, uint16_t bc);

/**
 * @brief 更新整个FLASH里的"storage"的字库
 *  @note 更新所有字体文件，包含UNIGBK,GBK12,GBK16,GBK24
 * @param xy:lcd显示坐标
 * @param size:lcd显示字体大小
 * @param src:字库来源磁盘
 *  @arg      "0:",SD卡
 *  @arg      "1:",FLASH盘
 * @param fc:lcd字体颜色
 * @param bc:lcd字体背景颜色
 * @return 0,成功；其他，错误代码
 */
uint8_t fonts_update_font(uint16_t x, uint16_t y, uint8_t size, uint8_t* src, uint16_t fc, uint16_t bc);

/**
 * @brief 检查字库是否存在，本身不会更新字库，并将分区表UNIGBK数据复制到RAM里
 * @return 0,字库存在；其他，字库不存在
 */
uint8_t fonts_init(void);
