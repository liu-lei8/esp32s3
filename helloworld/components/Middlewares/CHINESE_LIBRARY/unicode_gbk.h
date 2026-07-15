#pragma once /*unicode_gbk.h   UTF-8转unicode码，根据地址映射再从UNIGBK.BIN分区中找到GBK编码*/
/*deepseek学到的转换逻辑：https://chat.deepseek.com/share/fej31tekst476dcevj*/

#include <stdint.h>
#include "fonts.h"
#include <iconv.h>

/*UNIGBK.BIN 大小为 174344 字节
该文件的格式是一个unicode数据（小端序）+一个gbk数据（小端序）= 4字节，都是依次排序，但并不是按顺序排列的，所以无法用二叉查找法，只能从头逐个查找*/
#define UNIGBK_INDEX_SIZE   0x20000     /*131072字节，索引表大小*/
#define UNIGBK_DATA_OFFSET  UNIGBK_INDEX_SIZE   /*UNIGBK分区实际GBK编码数据的偏移位置*/

/**
 * @brief 将UTF-8编码转换为GBK编码，ASCII编码不影响
 * @param utf8:UTF-8下的字符串
 * @param gbk_buf:输出的GBK编码值的指针
 * @param buf_len:缓冲区长度（字节）
 * @retval: 实际写入长度（字节）
 */
int utf8_to_gbk(const char* utf8, uint8_t* gbk_buf, int buf_len);

/**
 * @brief UNIGBK测试函数，观察起始数据和中间附近的数据，以及对比四个字符unicode和gbk码是否对应
 */
void unigbk_test(void);