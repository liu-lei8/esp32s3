#pragma once

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <assert.h>

#define DS18B20_DQ_PIN      GPIO_NUM_0

/**
 * 分辨率由暂存器 第 5 字节（配置寄存器） 的 第 5 位 (R0) 和 第 6 位 (R1) 控制
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

#define DS18B20_DQ_IN   gpio_get_level(DS18B20_DQ_PIN)
#define DS18B20_DQ_OUT(x)  do{x ? gpio_set_level(DS18B20_DQ_PIN, DQ_PIN_SET) :\
                              gpio_set_level(DS18B20_DQ_PIN, DQ_PIN_RESET);\
                            }while(0)

uint8_t ds18b20_init(void);
void ds18b20_reset(void);
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
void ds18b20_start_convert(void);