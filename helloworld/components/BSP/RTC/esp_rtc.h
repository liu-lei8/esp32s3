#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include <sys/time.h>       /*该头文件主要用于settimeofday和gettimeofday函数*/

typedef struct
{
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    /*公历年月日周*/
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t week;
}calendar_obj;

extern calendar_obj calendar;

/**
*@brief     RTC设置时间
*@param     year:年
*@param     mon:月
*@param     mday:日
*@param     hour:时
*@param     min:分
*@param     sec:秒
*@retval无
*/
void rtc_set_time(uint16_t year, uint8_t mon, uint8_t mday, uint8_t hour, uint8_t min, uint8_t sec);

/**
*@brief     获取当前的时间
*@param     无
*@retval    无
*/
void rtc_get_time(void);

/**
*@brief     将年月日时分秒转换成秒钟数
*@note      输入公历日期得到星期(起始时间为:公元0年3月1日开始,输入往后的任何日期,都可以获取正确的星期)
*           使用基姆拉尔森计算公式计算,原理说明见此贴:
*           https://www.cnblogs.com/fengbohello/p/3264300.html
*@param     year:年份
*@param     mon:月份
*@param     date:日期
*@retval    0,星期天;1~6:星期一~星期六
*/
uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t date);