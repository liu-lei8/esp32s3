#ifndef _PICLIB_H
#define _PICLIB_H

#include "lcd.h"
#include "bmp.h"
#include "gif.h"
#include "jpeg.h"
#include "png.h"
#include "exfuns.h"

#define PIC_FORMAT_ERR  0x27    /*格式错误*/
#define PIC_SIZE_ERR    0x28    /*图片尺寸错误*/
#define PIC_WINDOW_ERR  0x29    /*窗口设定错误*/
#define PIC_MEM_ERR     0x11    /*内存错误*/

/* 判断 TRUE 和 FALSE 是否已经定义了? 没有则要定义! */
#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

#define rgb565(r,g,b)   (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

/* 图片显示物理层接口 */
/* 在移植的时候,必须由用户自己实现这几个函数 */
typedef struct
{
    /* void draw_point(uint16_t x,uint16_t y,uint32_t color) 画点函数 */
    void(*draw_point)(uint16_t, uint16_t, uint16_t);
    
    /* void fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint32_t color) 单色填充函数 */
    void(*fill)(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t);
    
    /* void draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color) 画水平线函数 */
    void(*draw_hline)(uint16_t, uint16_t, uint16_t, uint16_t);
    
    /* void piclib_multi_color(uint16_t x, uint16_t y, uint16_t width, uint16_t *color) 多点填充 */
    void(*multicolor)(uint16_t, uint16_t, uint16_t, uint16_t *);
} _pic_phy;

extern _pic_phy pic_phy;

/*图像信息*/
typedef struct {
    uint16_t lcdwidth;      /*LCD宽度*/
    uint16_t lcdheight;     /*LCD高度*/
}_pic_info;

extern _pic_info picinfo;   /* 图像信息 */

/**
 * @brief 画图初始化
 *  @note 画图之前，必须先调用此函数，指定相关函数
 */
void piclib_init(void);
/**
 * @brief   智能画图
 *  @note   图片仅在x，y和width,height限定区域显示
 * @param filename: 包含路径的文件名(.bmp/.jpeg/.png/.gif等)
 * @param x,y: 起始坐标
 * @param width,height: 显示区域
 * @retval 如果传入的是GIF图片，0：成功，其他：失败。传入其他格式的图片返回值都表示解码速度
 */
uint32_t piclib_ai_load_picfile(char* filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height);

/**
 * @brief 动态分配内存，在解压文件gif.c里有用到
 * @param size:要申请的内存大小（字节）
 * @return 分配到内存的首地址
 */
void* piclib_mem_malloc(uint32_t size);
void piclib_mem_free(void* paddr);

#endif