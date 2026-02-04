#pragma once

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
                        /*IIC总线的数据线和时钟线各加一个外部上拉电阻*/
/*IIC控制块*/
typedef struct {
    i2c_port_t port;
    gpio_num_t scl;
    gpio_num_t sda;
    esp_err_t init_flag;
}i2c_obj_t;

/*读写数据缓冲区结构体*/
typedef struct {
    size_t len;
    uint8_t* buf;
}i2c_buf_t;

extern i2c_obj_t iic_master[I2C_NUM_MAX];

/*读写标志位*/
#define I2C_FLAG_READ               (0X01)      /*读标志*/
#define I2C_FLAG_STOP               (0X02)      /*停止标志*/
#define I2C_FLAG_WRITE              (0X04)      /*写标志*/

/*引脚与相关参数定义*/
#define IIC0_SDA_GPIO_PIN           GPIO_NUM_41 /*IIC0_SDA引脚*/
#define IIC0_SCL_GPIO_PIN           GPIO_NUM_42 /*IIC0_SCL引脚*/
#define IIC1_SDA_GPIO_PIN           GPIO_NUM_5
#define IIC1_SCL_GPIO_PIN           GPIO_NUM_4
#define IIC_FREQ                    400000      /*IIC通信频率*/
#define I2C_MASTER_TX_BUF_DISABLE   0           /*I2C主机不需要发送缓冲区*/
#define I2C_MASTER_RX_BUF_DISABLE   0           /*I2C主机不需要接收缓冲区*/
#define ACK_CHECK_EN                0x1         /*I2C master将从slave检查ACK*/

i2c_obj_t iic_init(uint8_t iic_port);
esp_err_t i2c_transfer(i2c_obj_t* self, uint16_t addr, size_t n, i2c_buf_t* bufs, unsigned int flags);