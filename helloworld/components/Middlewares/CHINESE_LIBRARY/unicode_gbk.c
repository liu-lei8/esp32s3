#include "unicode_gbk.h"

/**
 * @brief utf8编码转unicode码点
 * @param utf8:utf8编码值
 * @param unicode:输出的码点
 * @retval: 1,2,3,4:UTF-8编码的字节数，-1非法编码
 */
static int utf8_to_unicode(const uint8_t* utf8, uint16_t* unicode)
{
    /*分支1：单字节字符（0xxxxxxx）ASCII 区*/
    if (utf8[0] <= 0x7F)
    {
        *unicode = utf8[0];
        return 1;
    }
    /*分支2：双字节模板（110xxxxx 10xxxxxx）拉丁扩展、希腊字母等*/
    else if ((utf8[0] & 0xE0) == 0xC0)
    {
        if ((utf8[1] & 0xC0) != 0x80) return -1;
        /*取UTF8的高字节的低5位和低字节的低6位组成unicode编码*/
        *unicode = ((utf8[0] & 0x1F) << 6 ) | (utf8[1] & 0x3F);
        return 2;
    }
    /*分支3：三字节模板（1110xxxx 10xxxxxx 10xxxxxx）中文、日文等*/
    else if ((utf8[0] & 0xF0) == 0xE0)
    {
        if ((utf8[1] & 0xC0) != 0x80 || (utf8[2] & 0xC0) != 0x80) return -1;
        *unicode = ((utf8[0] & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F);
        return 3;
    }
    /*分支4：四字节模板（11110xxx 10xxxxxx 10xxxxxx 10xxxxxx）补充平面，如 emoji*/
    else if ((utf8[0] & 0xF8) == 0xF0)
    {
        if ((utf8[1] & 0xC0) != 0x80 || (utf8[2] & 0xC0) != 0x80 || (utf8[3] & 0xC0) != 0x80) return -1;
        *unicode = ((utf8[0] & 0x07) << 18) | ((utf8[1] & 0x3F) << 12) |
                   ((utf8[2] & 0x3F) << 6)  | (utf8[3] & 0x3F);
        return 4;
    }

    return -1;
}

/**
 * @brief 码点转换为GBK编码(利用了二叉法查找，但是UNIGBK.BIN文件不是按顺序排列的所以这个无法使用)
 * @param unicode:输入的码点
 * @param gbk:输出的GBK编码指针
 * @return 0,成功 ; 其他,失败
 */
static int unicode_to_gbk(uint16_t unicode, uint8_t *gbk)
{
    uint32_t size = ftinfo.ugbksize;
    if (size < 4) return -1;

    uint32_t count = size / 4;          // 总条目数
    uint32_t low = 0, high = count - 1;
    uint8_t buf[4];
    uint16_t cur_unicode;

    while (low <= high) {
        uint32_t mid = low + (high - low) / 2;
        uint32_t addr = ftinfo.ugbkaddr + mid * 4;

        if (fonts_partition_read(addr, buf, 4) != ESP_OK)
            return -1;

        // 小端解码 Unicode
        cur_unicode = buf[0] | (buf[1] << 8);

        if (cur_unicode == unicode) {
            // 找到，GBK 为后两字节，小端序转大端序
            gbk[0] = buf[3];
            gbk[1] = buf[2];
            return 0;
        } else if (cur_unicode < unicode) {
            low = mid + 1;
        } else {
            if (mid == 0) break;
            high = mid - 1;
        }
    }
    return -1;   // 未找到映射
}

/**
 * @brief 码点转换为GBK编码(按顺序逐个查找法，直接在分区表里查找，打印汉字会很慢)
 * @param unicode:输入的码点
 * @param gbk:输出的GBK编码指针
 * @return 0,成功 ; 其他,失败
 */
static int unicode_to_gbk_1(uint16_t unicode, uint8_t* gbk)
{
    uint32_t count = ftinfo.ugbksize / 4;   /*总条目*/
    uint8_t buf[4];
    uint16_t cur_unicode;

    for (uint32_t i = 0; i < count; i++)
    {
        fonts_partition_read(ftinfo.ugbkaddr + i * 4, buf, 4);
        cur_unicode = buf[0] | (buf[1] << 8);   /*小端序转大端*/
        if (cur_unicode == unicode)             /*找到了 */
        {
            gbk[0] = buf[3];                    /*小端序转大端*/
            gbk[1] = buf[2];
            return 0;
        }
    }
    return -1;
}

/**
 * @brief 码点转换为GBK编码(按顺序逐个查找法，通过fonts_init函数从分区表复制到RAM里查找，速度快)
 * @param unicode:输入的码点
 * @param gbk:输出的GBK编码指针
 * @return 0,成功 ; 其他,失败
 */
static int unicode_to_gbk_2(uint16_t unicode, uint8_t *gbk)
{
    if (unigbk_table == NULL) return -1;

    uint32_t count = ftinfo.ugbksize / 4;
    uint8_t *rec = unigbk_table;

    for (uint32_t i = 0; i < count; i++) {
        uint16_t cur_unicode = rec[0] | (rec[1] << 8);
        if (cur_unicode == unicode) {
            gbk[0] = rec[3];   // 小端 GBK 转大端
            gbk[1] = rec[2];
            return 0;
        }
        rec += 4;
    }
    return -1;   // 未找到
}

int utf8_to_gbk(const char* utf8, uint8_t* gbk_buf, int buf_len)
{
    int out_len = 0;
    uint16_t unicode;
    int bytes_len;
    uint8_t gbk[2];
    const uint8_t* p = (const uint8_t*)utf8;

    while (*p && out_len <= buf_len)
    {
        bytes_len = utf8_to_unicode(p, &unicode);
        if (bytes_len <= 0) break;  /*解码失败，终止*/

        if (unicode <= 0x7F) gbk_buf[out_len++] = (uint8_t)unicode; /*ASCII直接拷贝*/
        else                                                        /*汉字*/
        {
            if (unicode_to_gbk_2(unicode, gbk) == 0)
            {
                if (out_len + 2 <= buf_len)
                {
                    gbk_buf[out_len++] = gbk[0];
                    gbk_buf[out_len++] = gbk[1];
                }
                else break; /*缓冲区不足*/
            }
            else    /*找不到对应字符，填充全角空格*/
            {
                 if (out_len + 2 <= buf_len)
                {
                    gbk_buf[out_len++] = 0xA1;
                    gbk_buf[out_len++] = 0xA1;
                }
                else break;
            }
        }

        p += bytes_len;
    }

    /*添加终止符 */
    if (out_len < buf_len)
    {
        gbk_buf[out_len] = '\0';
    }
    else
    {
        gbk_buf[buf_len - 1] = '\0';
    }

    return out_len;
}

void unigbk_test(void)
{
    uint8_t gbk[2];
    uint16_t tests[] = {0x6B63, 0x4E2D, 0x575F, 0x00A4}; // 正, 中, 坟, ¤

    printf("unigbk_table loaded, first 4 bytes: %02X %02X %02X %02X\n", unigbk_table[0], unigbk_table[1], unigbk_table[2], unigbk_table[3]);

    for (int i = 0; i < 4; i++) {
        if (unicode_to_gbk_2(tests[i], gbk) == 0) {
            printf("Unicode 0x%04X -> GBK 0x%02X%02X\n", tests[i], gbk[0], gbk[1]);
        } else {
            printf("Unicode 0x%04X -> NOT FOUND\n", tests[i]);
        }
    }

    if (unigbk_table) {
        // 探查索引 21792 附近的数据
        uint32_t base = 21790 * 4;  // 从 21790 开始打印几条
        printf("RAW data near index 21792:\n");
        for (int i = 0; i < 5; i++) {
            uint32_t off = base + i * 4;
            printf("  idx %5u: %02X %02X %02X %02X\n",
                   21790 + i,
                   unigbk_table[off], unigbk_table[off+1],
                   unigbk_table[off+2], unigbk_table[off+3]);
        }
    }
}
