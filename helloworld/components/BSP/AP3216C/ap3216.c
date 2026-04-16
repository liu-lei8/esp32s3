#include "ap3216.h"

i2c_obj_t ap3216c_master;

static esp_err_t ap3216c_read_one_byte(uint8_t reg, uint8_t* data)
{
    i2c_buf_t buf[2] = {
        {.buf = &reg, .len = 1},
        {.buf = data, .len = 1}
    };

    esp_err_t ret = iic_transfer(&ap3216c_master, AP3216C_ADDR, 2, buf, IIC_FLAG_WRITE | IIC_FLAG_READ | IIC_FLAG_STOP);

    return ret;
}

static esp_err_t ap3216c_write_one_byte(uint8_t reg, uint8_t data)
{
    i2c_buf_t buf[2] = {
        {.buf = &reg, .len = 1},
        {.buf = &data, .len = 1},
    };

    esp_err_t ret = iic_transfer(&ap3216c_master, AP3216C_ADDR, 2, buf, IIC_FLAG_STOP);

    return ret;
}

void ap3216c_init(i2c_obj_t self)
{
    if (self.init_flag == ESP_FAIL)
    {
        ap3216c_master = iic_init(I2C_NUM_0);
    }
    else
    {
        ap3216c_master = self;
    }

    while(ap3216c_config())
    {
        ESP_LOGE("ap3216c", "ap3216c init fail!!!");
        vTaskDelay(500);
    }
}

uint8_t ap3216c_config(void)
{
    uint8_t temp;

    ap3216c_write_one_byte(0x00, 0x04);     /*复位ap3216c*/
    vTaskDelay(50);                         /*复位至少需要10ms*/
    ap3216c_write_one_byte(0x00, 0x03);     /*开启ALS,IR+PS*/
    ap3216c_read_one_byte(0x00, &temp);     /*读取刚刚写进去的0x03，验证是否通信成功*/

    if (temp == 0x03)
    {
        ESP_LOGI("ap3216c", "ap3216c config success!");
        return 0;
    }
    else
    {
        ESP_LOGE("ap3216c", "ap3216c config fail!");
        return 1;
    }
}

void ap3216c_read_data(uint16_t* ir, uint16_t* als, uint16_t* ps)
{
    uint8_t buf[6];

    for (uint8_t i = 0; i < 6; i++)
    {
        ap3216c_read_one_byte(0x0A + i, &buf[i]);       /*循环读取所有传感器数据*/
    }

    /*验证IR_OF（output fail）位和PS_OF位都为1时，说明两个数据均无效*/
    if ((buf[0] & 0x80) && (buf[4] & 0x40) && (buf[5] & 0x40))
    {
        *ir = 0;
        *ps = 0;
    }
    else
    {
        *ir = ((uint16_t)buf[1] << 2) | (buf[0] & 0x03);
        *ps = ((uint16_t)(buf[5] & 0x3F) << 4) | (buf[4] & 0x0F);
    }

    *als = ((uint16_t)buf[3] << 8) | buf[2];
}
