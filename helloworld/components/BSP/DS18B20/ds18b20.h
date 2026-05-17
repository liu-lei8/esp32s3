#pragma once

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <assert.h>
#include <string.h>

#define DS18B20_DQ_PIN      GPIO_NUM_0

/**分辨率由暂存器 第 5 字节（配置寄存器） 的 第 5 位 (R0) 和 第 6 位 (R1) 控制
 * 9 位分辨率 (R1=0, R0=0)：保留位全为 1，所以配置字节是 0001 1111，即 0x1F。
 * 10 位分辨率 (R1=0, R0=1)：配置字节是 0011 1111，即 0x3F。
 * 11 位分辨率 (R1=1, R0=0)：配置字节是 0101 1111，即 0x5F。
 * 12 位分辨率 (R1=1, R0=1)：配置字节是 0111 1111，即 0x7F。
 */
#define DS18B20_RES_9_BIT   0x1F    /*最大转化时间93.75ms，温度分度0.5℃*/
#define DS18B20_RES_10_BIT  0x3F    /*最大转化时间187.5ms，温度分度0.25℃*/
#define DS18B20_RES_11_BIT  0x5F    /*最大转化时间375ms，温度分度0.125℃*/
#define DS18B20_RES_12_BIT  0x7F    /*最大转化时间750ms，温度分度0.0625℃(默认)*/

extern const uint8_t ds18b20_rom0[8];
extern const uint8_t ds18b20_rom1[8];

/*DS18B20的DQ引脚高低电平枚举*/
typedef enum
{
    DQ_PIN_RESET = 0u,
    DQ_PIN_SET,
}DS18B20_DQ_PinState;

/*DS18B20传感器信息结构体*/
typedef struct{
    const uint8_t* rom;         /*64位序列号*/
    short low_threshold;        /*低温报警阈值（温度✖️10）*/
    short high_threshold;       /*高温报警阈值（温度✖️10）*/
    short current_temp;         /*当前温度（温度✖️10）*/
    uint8_t alarm_flag : 1;     /*超温报警标志*/
    uint8_t high_ararm : 1;     /*高温报警标志*/
    uint8_t low_alarm  : 1;     /*低温报警标志*/
}ds18b20_sensor_t;

extern ds18b20_sensor_t sensor_A;
extern ds18b20_sensor_t sensor_B;
extern ds18b20_sensor_t* sensors[];

#define DS18B20_COUNT 2

#define DS18B20_DQ_IN   gpio_get_level(DS18B20_DQ_PIN)
#define DS18B20_DQ_OUT(x)  do{x ? gpio_set_level(DS18B20_DQ_PIN, DQ_PIN_SET) :\
                              gpio_set_level(DS18B20_DQ_PIN, DQ_PIN_RESET);\
                            }while(0)

uint8_t ds18b20_init(void);
void ds18b20_reset(void);
/**
 * @brief 用于对ds18b20复位之后检查它的应答信号
 * @return 0:成功检测到应答，1：未检测到应答，通信失败
 */
uint8_t ds18b20_check(void);
/**
 * @brief   获取温度✖️10（-55~125℃），该函数只能用于在总线上挂着一个设备
 */
short ds18b20_get_temperature(void);
/**
 * @brief   用于获取某一个ds18b20设备的温度
 * @note    使用该函数之前必须先使用ds18b20_start_convert这个函数转化
 * @param rom 指向ds18b20的64位序列号的指针
 * @return 返回获取的温度✖️10（-55~125℃）
 */
short ds18b20_get_temperature_rom(const uint8_t* rom);
/**
 * @brief   读取单个DS18B20的64位rom序列号
 * @note    使用此函数时，务必确保单总线上只有一个传感器
 */
void ds18b20_read_rom(void);
/**
 * @brief 对单总线上所有ds18b20设备发送模数转化命令
 */
void ds18b20_start_convert(void);
/**
 * @brief  设置指定传感器的报警阈值（单位：℃）
 * @param  rom        : 传感器 8 字节序列号
 * @param  high_temp  : 高温阈值（℃），范围 -55 ~ 125
 * @param  low_temp   : 低温阈值（℃），范围 -55 ~ 125
 * @note   阈值是按 1℃ 步进的整数，TH 必须 > TL
 */
void ds18b20_set_alarm(const uint8_t* rom, int8_t low_alarm, int8_t high_alarm);
/**
 * @brief  执行报警搜索，识别报警的传感器
 * @note   会更新全局传感器结构体中的 alarm 标志
 * @return 返回报警设备的数量
 */
int ds18b20_alarm_search(void);