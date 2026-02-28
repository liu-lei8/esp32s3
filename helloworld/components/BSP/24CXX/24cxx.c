#include "24cxx.h"

i2c_obj_t at24cxx_master;

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
}

uint8_t at24cxx_read_one_byte(uint16_t addr)
{
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
