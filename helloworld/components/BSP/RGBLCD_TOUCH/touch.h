#pragma once    /*touch.h*/

#include "ltdc.h"
#include "gt9xxx.h"
#include "led.h"

#define TP_PRES_DOWN    0X8000      /*触屏被按下*/
#define TP_CATH_PRES    0X4000      /*有按键按下*/
#define CT_MAX_TOUCH    10          /*电容屏支持的最大触点数量*/

/*触摸屏控制器*/
typedef struct
{
    uint8_t (*init)(i2c_obj_t);      /*初始化屏幕控制器*/
    uint8_t (*scan)(uint8_t);   /*扫描触摸屏，参数为了兼容电阻屏，电容屏未用到填0即可*/
    uint16_t x[CT_MAX_TOUCH];   /*当前坐标*/
    uint16_t y[CT_MAX_TOUCH];   /*电容屏最多十组坐标，第一个触点用x[0],y[0]表示，用x[9],y[9]储存第一次按下的坐标*/

    uint16_t sta;               /* 触摸状态
                                 * b15：按下1/松开0
                                 * b14：1，有按键按下；0，没有按键按下
                                 * b13-10：保留
                                 * b9-b0：每一位代表十个触摸点是否按下（1按下，0未按下）*/

    /*5点校准触摸屏校准参数（电容屏不需要校准）*/
    float xfac;                 /*5点校准法x方向比例因子*/
    float yfac;                 /*5点校准法y方向比例因子*/
    short xc;                   /*中心x坐标物理值（AD值）*/
    short yc;                   /*中心y坐标物理值（AD值）*/

    /**新增的参数，当触摸屏上下左右完全颠倒时需要用到
     * b0:0，竖屏（适合左右为Y坐标，上下为X坐标的TP）
     *    1，横屏（适合左右为X坐标，上下为Y坐标的TP）
     * b1~b6：保留
     * b7:0，电阻屏
     *    1，电容屏
    */
   uint8_t touchtype;
}_m_tp_dev;

extern _m_tp_dev tp_dev;

/**
 * @brief 电容触摸屏的初始化
 * @note 给触摸屏控制块赋值以及函数，并初始化gt9xxx
 * @return 0,初始化成功;1,初始化失败
 */
uint8_t ctp_init(i2c_obj_t self);

/**
 * @brief 清屏并显示RST清屏触摸位置
 */
void load_draw_dialog(void);

/**
 * @brief 画粗线
 * @param size：线的粗细
 */
void ctp_draw_bline(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint8_t size, uint16_t color);

/**
 * @brief 死循环中根据扫描到的触摸点画粗线
 */
void ctp_test(void);
