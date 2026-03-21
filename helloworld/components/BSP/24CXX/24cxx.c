#include "24cxx.h"

i2c_obj_t at24cxx_master;
extern SemaphoreHandle_t i2c_mutex;

void at24cxx_init(i2c_obj_t self)
{
    if (self.init_flag == ESP_FAIL)
    {
        at24cxx_master = iic_init(I2C_NUM_0);
    }
    else
    {
        at24cxx_master = self;
    }
}

void at24cxx_write_one_byte(uint16_t addr, uint8_t data)
{
    xSemaphoreTake(i2c_mutex, portMAX_DELAY);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    if (EE_TYPE > AT24C16)
    {
        ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (AT_ADDR << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN));
        ESP_ERROR_CHECK(i2c_master_write_byte(cmd, addr >> 8, ACK_CHECK_EN));        //先发送内存地址高位
    }
    else
    {
        /*AT24C16的内存地址高位刚好占用设备地址的后3个可编程位*/
        ESP_ERROR_CHECK(i2c_master_write_byte(cmd, 0XA0 + ((addr / 256) << 1), ACK_CHECK_EN));
    }

    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, addr % 256, ACK_CHECK_EN));           //发送内存地址低位
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, data, ACK_CHECK_EN));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    ESP_ERROR_CHECK(i2c_master_cmd_begin(at24cxx_master.port, cmd, 5000));
    i2c_cmd_link_delete(cmd);

    vTaskDelay(10);
    xSemaphoreGive(i2c_mutex);
}

uint8_t at24cxx_read_one_byte(uint16_t addr)
{
    xSemaphoreTake(i2c_mutex, portMAX_DELAY);
    uint8_t data = 0;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    if (EE_TYPE > AT24C16)
    {
        i2c_master_write_byte(cmd, (AT_ADDR << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, (addr >> 8), ACK_CHECK_EN);
    }
    else
    {
        i2c_master_write_byte(cmd, 0XA0 + ((addr / 256) << 1), ACK_CHECK_EN);
    }

    i2c_master_write_byte(cmd, addr % 256, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (AT_ADDR << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(at24cxx_master.port, cmd, 1000);
    i2c_cmd_link_delete(cmd);

    vTaskDelay(10);
    xSemaphoreGive(i2c_mutex);
    return data;
}

esp_err_t at24cxx_check(void)
{
    uint8_t temp;
    uint16_t addr = EE_TYPE;
    temp = at24cxx_read_one_byte(addr);     /*先读，避免每次重新写入*/

    if (temp == 0x55)
    {
        return ESP_OK;
    }
    else
    {
        at24cxx_write_one_byte(addr, 0x55);         /*第一次写入，后续不再写入*/
        temp = at24cxx_read_one_byte(addr);
        if (temp == 0x55)
        {
            return ESP_OK;
        }
    }

    return ESP_FAIL;
}

void at24cxx_write(uint16_t addr, uint8_t* pbuf, uint16_t datalen)
{
    while (datalen--)
    {
        at24cxx_write_one_byte(addr, *pbuf);
        addr++;
        pbuf++;
    }
}

void at24cxx_read(uint16_t addr, uint8_t* pbuf, uint16_t datalen)
{
    while (datalen--)
    {
        *pbuf++ = at24cxx_read_one_byte(addr++);
    }
}

void at24cxx_write_serial(uint16_t addr, uint8_t* pbuf, uint16_t datalen)
{
    i2c_buf_t bufs[2] = { 0 };
    uint8_t Addr[2] = {addr >> 8, addr % 256};      /*将内存地址拆分为高八位和低八位*/

    if (EE_TYPE > AT24C16)
    {
        //bufs[0] = (i2c_buf_t){.buf = 's', .len = 1};   定义后可以使用复合字面量赋值，必须加强制类型转换
        bufs[0].buf = Addr;
        bufs[0].len = 2;
        bufs[1].buf = pbuf;
        bufs[1].len = (size_t)datalen;
        /*如果bufs是指针的话，可以按下面的方式赋值*/
        //bufs = (i2c_buf_t [2]){{.buf = Addr, .len = 2}, {.buf = pbuf, .len = (size_t)datalen}};
        iic_transfer(&at24cxx_master, AT_ADDR, 2, bufs, IIC_FLAG_STOP);
    }
    else
    {
        bufs[0].buf = addr % 256;
        bufs[0].len = 1;
        bufs[1].buf = pbuf;
        bufs[1].len = (size_t)datalen;
        /*发送设备地址和高位内存地址不用左移, 在函数内部左移*/
        iic_transfer(&at24cxx_master, AT_ADDR + (addr / 256), 2, bufs, IIC_FLAG_STOP);
    }
    vTaskDelay(10);
}

void at24cxx_read_serial(uint16_t addr, uint8_t* pbuf, uint16_t datalen)
{
    i2c_buf_t bufs[2] = { 0 };
    uint8_t Addr[2] = {addr >> 8, addr % 256};

    if (EE_TYPE > AT24C16)
    {
        bufs[0].buf = Addr;
        bufs[0].len = 2;
        bufs[1].buf = pbuf;
        bufs[1].len = (size_t)datalen;
        iic_transfer(&at24cxx_master, AT_ADDR, 2, bufs, IIC_FLAG_STOP | IIC_FLAG_READ | IIC_FLAG_WRITE);
    }
    else
    {
        bufs[0].buf = addr % 256;
        bufs[0].len = 1;
        bufs[1].buf = pbuf;
        bufs[1].len = (size_t)datalen;
        iic_transfer(&at24cxx_master, AT_ADDR + (addr / 256), 2, bufs, IIC_FLAG_STOP | IIC_FLAG_READ | IIC_FLAG_WRITE);
    }
    vTaskDelay(10);
}
