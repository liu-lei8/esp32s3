#pragma once

/*常用ASCII表
*偏移量32
*ASCII字符集: !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]
^_`abcdefghijklmnopqrstuvwxyz{|}~
*PC2LCD2002取模方式设置：阴码+逐列式+顺向+C51格式
*总共：4个字符集（12*12、16*16、24*24和32*32），用户可以自行新增其他分辨率的字符集。
*每个字符所占用的字节数为:(size/8+((size%8)?1:0))*(size/2),其中size:是字库生成时的点
*阵大小(12/16/24...)
*/

/*12*12ASCII字符集点阵*/
extern const unsigned char lcd_asc2_1206[95][12];
/*16*16ASCII字符集点阵*/
extern const unsigned char lcd_asc2_1608[95][16];
/*24*24ASCII字符集点阵*/
extern const unsigned char lcd_asc2_2412[95][36];
/*32*32ASCII字符集点阵*/
extern const unsigned char lcd_asc2_3216[95][64];

typedef struct
{
    unsigned char index[3];     /*索引用于寻找字体，UTF-8一个汉字三个字节*/
    unsigned char mask[128];
}typFNT_GB32;

/*32*32中文字符集点阵*/
extern const typFNT_GB32 tfont32[8];

/*280*240微笑表情*/
extern const unsigned char picture0[];