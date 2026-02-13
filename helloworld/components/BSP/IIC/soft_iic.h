#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"        /*方案1.用于微秒级延迟函数esp_rom_delay_us*/
#include <unistd.h>             /*方案2.用于c中的微秒级延迟函数usleep*/

typedef struct {
    gpio_num_t scl_pin;
    gpio_num_t sda_pin;
    uint32_t frequency;     /*时钟频率*/
    uint32_t delay_us;      /*高低电平延迟时间微秒数（通过频率计算）*/
    bool initialized;
}soft_iic_t;

#define SOFT_IIC_SCL_PIN    GPIO_NUM_42
#define SOFT_IIC_SDA_PIN    GPIO_NUM_41
#define SOFT_IIC_FREQ       100000

/*函数设计逻辑：
1.前期的初始化(iic控制块初始化+引脚初始化+总线初始化)和反初始化
2.SCL和SDA的电平设置以及读取SDA电平+高低电平延迟设置 
3.起始信号+（写一个字节数据、写数据）+（读一个字节数据、读数据）+停止信号
4.检查从机设备*/

soft_iic_t* soft_iic_init(gpio_num_t scl_pin, gpio_num_t sda_pin, uint32_t frequency);

void soft_iic_deinit(soft_iic_t* iic);

void soft_iic_start(soft_iic_t* iic);

void soft_iic_stop(soft_iic_t* iic);

uint8_t soft_iic_send_byte(soft_iic_t* iic, uint8_t data);

uint8_t soft_iic_receive_byte(soft_iic_t* iic, bool ack);

esp_err_t soft_iic_write(soft_iic_t* iic, uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t len);

esp_err_t soft_iic_read(soft_iic_t* iic, uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t len);

esp_err_t soft_iic_check_device(soft_iic_t* iic, uint8_t dev_addr);

void soft_iic_set_scl(soft_iic_t* iic, bool level);

void soft_iic_set_sda(soft_iic_t* iic, bool level);

uint32_t soft_iic_read_sda(soft_iic_t* iic);

void soft_iic_delay(soft_iic_t* iic);