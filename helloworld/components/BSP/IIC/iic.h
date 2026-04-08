#pragma once

#include "driver/i2c.h"
#include "esp_log.h"


typedef struct {
    i2c_port_t port;
    gpio_num_t scl;
    gpio_num_t sda;
    esp_err_t init_flag;
}i2c_obj_t;

typedef struct {
    uint8_t* buf;
    size_t len;
}i2c_buf_t;

/*读写标志位*/
#define IIC_FLAG_READ   0x01
#define IIC_FLAG_STOP   0x02
#define IIC_FLAG_WRITE  0x04

/*引脚与相关参数定义*/
#define IIC0_SDA_GPIO_PIN           GPIO_NUM_8
#define IIC0_SCL_GPIO_PIN           GPIO_NUM_9
#define IIC1_SDA_GPIO_PIN           GPIO_NUM_4
#define IIC1_SCL_GPIO_PIN           GPIO_NUM_5
#define IIC_FREQ                    400000
#define IIC_MASTER_TX_BUF_DISABLE   0
#define IIC_MASTER_RX_BUF_DISABLE   0
#define ACK_CHECK_EN                0x1

i2c_obj_t iic_init(i2c_port_t port);
esp_err_t iic_transfer(i2c_obj_t* self, uint16_t addr, size_t n, i2c_buf_t* bufs, unsigned int flags);
void iic_scan(i2c_obj_t self);