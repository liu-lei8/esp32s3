#include "esp_rtc.h"

calendar_obj calendar;

void rtc_set_time(uint16_t year, uint8_t mon, uint8_t mday, uint8_t hour, uint8_t min, uint8_t sec)
{
    struct tm datetime = {
        .tm_year = year - 1900,
        .tm_mon = mon - 1,
        .tm_mday = mday,        /*yday是一年中的第几天，wday是星期几0-6,0是星期天*/
        .tm_hour = hour,
        .tm_min = min,
        .tm_sec = sec,
        .tm_isdst = -1,         /*夏令时标志: >0 表示生效，0 表示不生效，<0 表示未知*/
    };

    time_t second = mktime(&datetime);  /*获得自1900.1.1以来的总秒数*/
    struct timeval val = {.tv_sec = second, .tv_usec = 0};
    /*给RTC定时器设置当前时间,该函数是依赖库newlib的C库的一部分*/
    settimeofday(&val, NULL);   /*第一个参数用于设置秒和毫秒，第二个参数设置时区*/
}

void rtc_get_time(void)
{
    struct tm* datetime;
    time_t second;
    /*返回自（1900.1.1 00:00:00 UTC）经过的时间（秒）,也就是获得当前的时间戳*/
    time(&second);
    datetime = localtime(&second);

    calendar.hour = datetime->tm_hour;
    calendar.min = datetime->tm_min;
    calendar.sec = datetime->tm_sec;
    calendar.year = datetime->tm_year + 1900;
    calendar.month = datetime->tm_mon + 1;
    calendar.date = datetime->tm_mday;

    calendar.week = rtc_get_week(calendar.year, calendar.month, calendar.date);
}

uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t date)
{
    uint8_t week = 0;
    if (month < 3)
    {
        month += 12;
        --year;
    }

    week = (date + 1 + (2 * month) + (3 * (month + 1) / 5) + year + (year >> 2) - (year / 100) + (year / 400)) % 7;

    return week;
}

/**
 * 以下为两个扩展函数：
 * 1.asctime()：该函数通过返回值将 struct tm 转换成如下格式的字符串（长度固定为26字节，包含换行符和结尾空字符）："Www Mmm dd hh:mm:ss yyyy\n\0"
 * 
 * 2.strftime()：该函数将struct tm转换成自定义的格式存储到自定义的缓冲区里，对应自定义的格式的年月日时分秒的数据转换查资料。
 * 
 * 常用格式说明符（节选）
说明符	    含义	            示例
%Y	        四位年份	        2026
%y	        两位年份	        26
%m	        月份（01-12）	    04
%d	        日（01-31）	        12
%H	        小时（00-23）	    15
%M	        分钟（00-59）	    08
%S	        秒（00-60）	        05
%A	        完整星期名	        Sunday
%a	        缩写星期名	        Sun
%B	        完整月份名	        April
%b 或 %h	缩写月份名	        Apr
%p	        AM/PM 标识	        AM
%I	        小时（01-12）	    03
%Z	        时区名称（如有）	 CST
%j	        年中的第几天（001-366）	102
%U	        年中的周数（周日为每周第一天）	15
%W	        年中的周数（周一为每周第一天）	15
%w	        星期几（0-6，0=周日）	0
%%	        百分号	%
 */