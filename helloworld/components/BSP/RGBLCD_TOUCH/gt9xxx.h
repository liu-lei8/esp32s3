#pragma once    /*gt9xxx.h*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iic.h"
#include "touch.h"
#include <string.h>

#define GT9XXX_INT_GPIO_PIN     GPIO_NUM_40     /*本实验未用到中断触发模式，而是用的轮询模式*/
#define GT9XXX_IIC_SDA          IIC0_SDA_GPIO_PIN     /*本实验数据时钟线应在iic.h文件定义*/
#define GT9XXX_IIC_SCL          IIC0_SCL_GPIO_PIN
#define CT_RST_PIN              GPIO_NUM_19
#define GT9XXX_INT              gpio_get_level(GT9XXX_INT_GPIO_PIN)
#define CT_RST(x)               do{x ? gpio_set_level(CT_RST_IO, 1):\
                                       gpio_set_level(CT_RST_IO, 0);\
                                }while(0)

#define GT9XXX_ADDR             0x14            /*7位从机设备地址*/

/*GT9xxx 部分寄存器定义*/
#define GT9XXX_CTRL_REG                 0X8040      /*GT9XXX控制寄存器*/
#define GT9XXX_CFGS_REG                 0X8047      /*GT9XXX配置寄存器起始地址，GT1151的起始地址是0x8050，该寄存器程序并没有用到，触摸屏出厂厂家已经设置好了*/
#define GT9XXX_CHECK_REG                0X80FF      /*GT9XXX校验和寄存器*/
#define GT9XXX_PID_REG                  0X8140      /*GT9XXX产品ID寄存器起始地址*/

#define GT9XXX_GSTID_REG                0X814E      /*GT9XXX当前检测到的触摸情况*/
#define GT9XXX_TP1_REG                  0X8150      /*第一个触摸点数据地址*/
#define GT9XXX_TP2_REG                  0X8158      /*第二个触摸点数据地址*/
#define GT9XXX_TP3_REG                  0X8160      /*第三个触摸点数据地址*/
#define GT9XXX_TP4_REG                  0X8168      /*第四个触摸点数据地址*/
#define GT9XXX_TP5_REG                  0X8170      /*第五个触摸点数据地址*/
#define GT9XXX_TP6_REG                  0X8178      /*第六个触摸点数据地址*/
#define GT9XXX_TP7_REG                  0X8180      /*第七个触摸点数据地址*/
#define GT9XXX_TP8_REG                  0X8188      /*第八个触摸点数据地址*/
#define GT9XXX_TP9_REG                  0X8190      /*第九个触摸点数据地址*/
#define GT9XXX_TP10_REG                 0X8198      /*第十个触摸点数据地址*/

extern i2c_obj_t gt9xxx_i2c_master;
extern uint8_t gt_tnum;

esp_err_t gt9xxx_write_reg(uint16_t reg, uint8_t* data, uint8_t len);
esp_err_t gt9xxx_read_reg(uint16_t reg, uint8_t* data, uint8_t len);

/**
 * @brief 初始化gt9xxx
 * @param self:iic控制块
 * @return 0,初始化成功；1,初始化失败
 */
uint8_t gt9xxx_init(i2c_obj_t self);

/**
 * @brief 扫描触摸屏获得坐标值以及触摸状态
 * @param mode:触摸屏未用到此参数填0即可，该参数为了兼容电阻屏
 * @return 1，有触摸；0，没有触摸
 */
uint8_t gt9xxx_scan(uint8_t mode);
