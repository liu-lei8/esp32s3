#pragma once

#include "freertos/FreeRTOS.h"
#include "iic.h"
#include "xl9555.h"

typedef struct 
{
    uint16_t ir;        /*Infrared Radiation*/
    uint16_t als;       /*Albient Light Sensor*/
    uint16_t ps;        /*Proximity Sensor*/
}ap3216c_value_t;

#define AP3216C_INT     xl9555_pin_read(AP_INT_IO)      /*本实验没有用到中断*/
#define AP3216C_ADDR    0x1E

void ap3216c_init(i2c_obj_t self);
/**
*@brief初始化AP3216C
*@param无
*@retval0,成功;
*1,失败;
*/
uint8_t ap3216c_config(void);

/**
*@brief 读取AP3216C的数据
*@note  读取原始数据，包括ALS,PS和IR
*如果同时打开ALS,IR+PS的话两次数据读取的时间间隔要大于112.5ms
*@param ir:IR传感器值
*@param ps:PS传感器值
*@param als:ALS传感器值
*@retval    无
*/
void ap3216c_read_data(uint16_t* ir, uint16_t* als, uint16_t* ps);