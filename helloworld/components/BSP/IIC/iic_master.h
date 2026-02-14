#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include <stdarg.h>

#define IIC_DEVICE_NUM_MAX  0x71        /*也就是iic总线一共可以挂载113种设备，从0x08-0x78的地址的从机设备*/
#define XL9555_DEVICE_NUM   0           /*iic上挂载的从机设备编号，用于从机设备句柄的索引*/
/*读写标志位*/
#define IIC_FLAG_READ   0x01
#define IIC_FLAG_STOP   0x02            /*在新版API中可以不用要了*/
#define IIC_FLAG_WRITE  0x04

/*引脚与相关参数定义*/
#define IIC0_SDA_GPIO_PIN           GPIO_NUM_41
#define IIC0_SCL_GPIO_PIN           GPIO_NUM_42
#define IIC1_SDA_GPIO_PIN           GPIO_NUM_4
#define IIC1_SCL_GPIO_PIN           GPIO_NUM_5
#define IIC_FREQ                    400000

typedef struct {
    i2c_port_t port;
    gpio_num_t scl;
    gpio_num_t sda;
    i2c_master_bus_handle_t bus_handle;     /*新增总线句柄*/
    i2c_master_dev_handle_t dev_handle[IIC_DEVICE_NUM_MAX];
    esp_err_t init_flag;
}iic_obj_t;

typedef struct {
    uint8_t* buf;
    size_t len;
}iic_buf_t;


/*
初始化iic
参数1：iic端口
参数2：iic总线上需要挂载的设备数量
参数3：可变参，能够填113种7位长的设备地址
*/
iic_obj_t* iic_init_new(i2c_port_t port, int device_num, ...);
esp_err_t iic_transfer_new(i2c_master_dev_handle_t dev_handle, iic_buf_t* bufs, uint8_t flags);
esp_err_t iic_delete_all_device(iic_obj_t* self, uint8_t device_num);