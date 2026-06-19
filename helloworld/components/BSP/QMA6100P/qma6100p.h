#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iic.h"
#include <math.h>

#define QMA6100P_ADDR   0x12    /*QMA6100P设备地址*/

/*QMA6100P命令*/

/*获取ID，默认值为0x9x*/
#define QMA6100P_REG_CHIP_ID        0x00

/*数据寄存器，三轴数据，默认为0x00 */
#define QMA6100P_REG_XOUTL          0x01
#define QMA6100P_REG_XOUTH          0x02
#define QMA6100P_REG_YOUTL          0x03
#define QMA6100P_REG_YOUTH          0x04
#define QMA6100P_REG_ZOUTL          0x05
#define QMA6100P_REG_ZOUTH          0x06

/*带宽寄存器*/
#define QMA6100P_REG_BW_ODR         0x10
/*电源管理寄存器*/
#define QMA6100P_REG_POWER_MANAGE   0x11
/*加速度范围寄存器，设置加速度计的满刻度*/
#define QMA6100P_REG_RANGE          0x0f
/*软件复位寄存器*/
#define QMA6100P_REG_RESET          0x36

/*QMA6100P 的加速度数据是 14 位的，但用了 16 位寄存器来存放，其中低 2 位是无效的（可能是噪声或未定义），所以需要先丢弃。*/
#define QMA6100P_REG_ACC_VAL(lsb, msb)  ((int16_t)(((uint16_t)msb << 8) | ((uint16_t)lsb & 0xFC)) >> 2)

#define M_G             9.80665f                /*标准重力加速度（m/s²）*/
//#define M_PI            3.141592653589793f      /*圆周率,math.h文件已定义了*/
#define RAD_TO_DEG      (180.0f / M_PI)         /*弧度转角度的转换因子*/

typedef struct{
    uint8_t data[2];
    float acc_x;
    float acc_y;
    float acc_z;
    float acc_g;
    float pitch;        /*围绕x轴旋转，也叫做俯仰角*/
    float roll;         /*围绕y轴旋转，也叫翻滚角*/
}qma6100p_rawdata_t;

/*设置带宽量程寄存器*/
typedef enum
{
    QMA6100P_BW_100 =   0,      /*输出数据速率（ODR）为100Hz*/
    QMA6100P_BW_200 =   1,
    QMA6100P_BW_400 =   2,
    QMA6100P_BW_800 =   3,
    QMA6100P_BW_1600 =  4,
    QMA6100P_BW_50 =    5,
    QMA6100P_BW_25 =    6,
    QMA6100P_BW_12_5 =  7,
    QMA6100P_BW_OTHER = 8,
}qma6100p_bw;

/*设置加速度范围寄存器*/
typedef enum
{
    QMA6100P_RANGE_2G =  0x01,
    QMA6100P_RANGE_4G =  0x02,
    QMA6100P_RANGE_8G =  0x04,
    QMA6100P_RANGE_16G = 0x08,
    QMA6100P_RANGE_32G = 0x0f,
}qma6100p_range;

/*设置复位寄存器*/
typedef enum
{
    QMA6100P_RESET =     0xB6,
    QMA6100P_RESET_END = 0x00,
}qma6100p_reset;

typedef enum
{
    QMA6100P_MAP_INT1,
    QMA6100P_MAP_INT2,
    QMA6100P_MAP_INT_NONE,
}qma6100p_int_map;

/*设置电源管理寄存器*/
typedef enum
{
    QMA6100P_ACTIVE =         0x80,
    QMA6100P_ACTIVE_DIGITAL = 0x84,
    QMA6100P_STANDBY =        0x00,
}qma6100p_power;

/*设置传感器内部的主时钟频率*/
typedef enum
{
    QMA6100P_MCLK_102_4K =   0x03,
    QMA6100P_MCLK_51_2K =    0x04,
    QMA6100P_MCLK_25_6K =    0x05,
    QMA6100P_MCLK_12_8K =    0x06,
    QMA6100P_MCLK_6_4K =     0x07,
    QMA6100P_MCLK_RESERVED = 0xff
}qma6100p_mclk;

/*灵敏度，数据换算系数，纯粹用于计算 */
typedef enum
{
    QMA6100P_SENSITIVITY_2G =  244,
    QMA6100P_SENSITIVITY_4G =  488,
    QMA6100P_SENSITIVITY_8G =  977,
    QMA6100P_SENSITIVITY_16G = 1950,
    QMA6100P_SENSITIVITY_32G = 3910
}qma6100p_sensitivity;

void qma6100p_init(i2c_obj_t self);
/**
 * @brief 读取qma6100p寄存器中原始的xyz三轴数据
 * @param data: 三轴数据存储数组
 */
void qma6100p_read_raw_xyz(int16_t data[3]);
/**
 * @brief 计算得到加速度计三轴xyz轴数据
 * @param accdata: 加速度计三轴存储数组
 */
void qma6100p_read_acc_xyz(float accdata[3]);
/**
 * @brief 获取加速度计三轴数据并计算姿态角
 * @param rawdata: 用于存储结果的结构体指针
 */
void qma6100p_read_rawdata(qma6100p_rawdata_t* rawdata);