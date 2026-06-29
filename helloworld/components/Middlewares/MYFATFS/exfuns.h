#pragma once /*exfuns.h*/

#include "esp_vfs_fat.h"
#include <string.h>
#include "lcd.h"
#include "key.h"

/*exfuns_file_type返回的类型定义
 * 根据表FILE_TYPE_TBL获得。在exfuns.c里面定义
 */
#define T_BIN   0x00    /*BIN二进制文件*/
#define T_LRC   0x10    /*LRC歌词文件*/
#define T_NES   0x20    /*NES复古游戏机的ROM文件*/
#define T_SMS   0x21    /*SMS复古游戏机的ROM文件*/
#define T_TEXT  0x30    /*TXT文本文件*/
#define T_C     0x31    /*C文件*/
#define T_H     0x32    /*H文件*/
#define T_WAV   0x40    /*WAV无损音频文件*/
#define T_MP3   0x41    /*MP3有损音频文件*/
#define T_APE   0x42    /*APE无损音频压缩文件*/
#define T_FLAC  0x43    /*FLAC免费的无损音频编码器文件*/
#define T_BMP   0x51    /*BMP无损位图*/
#define T_JPG   0x52    /*JPG有损压缩静态图像文件*/
#define T_JPEG  0x53    /*同上*/
#define T_GIF   0x54    /*GIF无损压缩，支持256色的多帧动态图，色彩丰富度差*/
#define T_PNG   0x55    /*PNG默认无损静态图，APNG支持动态图，是真彩色*/
#define T_AVI   0x60    /*AVI音视频文件*/

/**
 * @brief 为FatFS文件系统工作区分配内存
 * @return 0:成功，1：失败
 */
uint8_t exfuns_init(void);

/**
 * @brief 将小写字母转换为大写字母，如果是数字则保持不变
 * @param c:要转换的小写字母
 * @return 返回转换后的大写字母
 */
uint8_t exfuns_char_upper(uint8_t c);

/**
 * @brief 根据文件名获取文件的类型
 * @param fname:文件名
 * @return  返回文件的类型值
 *   @arg   类型值高四位表示所属大类，低四位表示所属小类
 *   @arg   0xFF表示无法识别的文件类型编号
 */
uint8_t exfuns_file_type(char* fname);

/**
 * @brief 获取磁盘总容量和剩余容量
 * @param pdrv:磁盘编号("0:"~"9:")
 * @param total:总容量(KB)
 * @param free:剩余容量(KB)
 * @return 0:正常，其他:错误代码
 */
uint8_t exfuns_get_free(uint8_t* pdrv, uint32_t* total, uint32_t* free);

/**
 * @brief   文件复制
 *  @note   将psrc文件copy到pdst文件
 * 
 * @param fcpymsg:函数指针，用于实现拷贝时的信息显示
 *          pname:文件/文件夹名
 *          pct:百分比
 *          mode:
 *              bit0:更新文件名
 *              bit1:跟新百分比
 *              bit2:跟新文件夹
 *              其他:保留
 *          返回值：0，正常；1，强制退出
 * 
 * @param psrc:源文件
 * @param pdst:目标文件
 * @param totsize:所有要复制文件的总大小（为0的时候，表示仅仅单个文件的复制）
 * @param cpdsize:以复制了的字节数
 * @param fwmode:目标文件写入模式
 *  @arg        0:不覆盖原有文件
 *  @arg        1：覆盖原有文件
 * 
 * @return 执行结果
 *  @arg   0:正常
 *  @arg   0xFF:强制退出
 *  @arg   其他：错误代码
 */
uint8_t exfuns_file_copy(uint8_t(*fcpymsg)(uint8_t* pname, uint8_t pct, uint8_t mode), uint8_t* psrc, uint8_t* pdst, uint32_t totsize, uint32_t cpdsize, uint8_t fwmode);

/**
 * @brief 获取路径下的文件夹名，即将路径丢掉只留文件夹名
 * @param pname:详细路径
 * @return 0    ,就是个卷编号
 *         其他 ,文件夹名首地址
 */
uint8_t* exfuns_get_src_dname(uint8_t* pname);

/**
 * @brief 得到文件夹大小
 *  @note 注意：文件夹大小不要超过4GB，因为返回值是uint32_t类型
 * @param fdname:文件夹详细路径
 * @retval 0    ,文件夹大小为0，或者程序过程发生错误
 *         其他 ,文件夹大小
 */
uint32_t exfuns_get_folder_size(uint8_t* fdname);

/**
 * @brief 拷贝文件夹
 *  @note 将psrc文件夹复制到pdst文件夹里
 *        注意：文件夹总大小不要超过4GB
 * 
 * @param fcpymsg:函数指针，用于拷贝时的信息显示
 *          pname:文件夹名/文件名
 *          pct:百分比
 *          mode:
 *              bit0:更新文件名
 *              bit1:更新百分比
 *              bit2:更新文件夹
 *              其他：保留
 *          返回值：0，正常；1，强制退出
 * 
 * @param psrc:源文件夹
 * @param pdst:目标文件夹
 *   @note      必须形如"X:"/"X:XX"/"X:XX/XX"之类的. 且要确认上一级文件夹存在
 * 
 * @param totsize:所有文件夹大小(供外部循环调用的)，如果totsize为0那么仅复制单个文件夹
 * @param cpdsize:已经复制了的大小
 * @param fwmode:文件写入模式
 *  @arg        0:不覆盖原有的文件
 *  @arg        1:覆盖原有文件
 * 
 * @retval      执行结果
 *  @arg        0:正常
 *  @arg        0xFF：强制退出
 *  @arg        其他：错误代码
 */
uint8_t exfuns_folder_copy(uint8_t(*fcpymsg)(uint8_t* pname, uint8_t pct, uint8_t mode), uint8_t* psrc, uint8_t* pdst, uint32_t* totsize, uint32_t* cpdsize, uint8_t fwmode);

/**
 * @brief 文件复制信息回调函数
 * @param pname:文件名/文件夹名
 * @param pct  :当前完成百分比
 * @param mode :调用模式标志位
 *              bit0:更新文件名
 *              bit1:更新百分比
 *              bit2:更新文件夹
 * @retval
 *  @arg 0:正常，继续复制
 *  @arg 1:强制退出复制
 */
uint8_t fcpymsg(uint8_t* pname, uint8_t pct, uint8_t mode);