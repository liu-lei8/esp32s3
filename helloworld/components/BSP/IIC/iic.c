#include "iic.h"

i2c_obj_t iic_master[I2C_NUM_MAX];

i2c_obj_t iic_init(uint8_t iic_port)
{
    uint8_t i;

    if (iic_port == I2C_NUM_0)
    {
        i = 0;
    }
    else
    {
        i = 1;
    }
    iic_master[i].port = iic_port;
    iic_master[i].init_flag = ESP_FAIL;

    if (iic_master[i].port == I2C_NUM_0)
    {
        iic_master[i].sda = IIC0_SDA_GPIO_PIN;
        iic_master[i].scl = IIC0_SCL_GPIO_PIN;
    }
    else
    {
        iic_master[i].sda = IIC1_SDA_GPIO_PIN;
        iic_master[i].scl = IIC1_SCL_GPIO_PIN;
    }

    i2c_config_t iic_config_struct = {
        //.clk_flags = I2C_CLK_SRC_DEFAULT,     /*在ESP-IDF v5.2中，不支持配置该选项*/
        .master.clk_speed = IIC_FREQ,
        .mode = I2C_MODE_MASTER,
        .scl_io_num = iic_master[i].scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .sda_io_num = iic_master[i].sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
    };
    i2c_param_config(iic_master[i].port, &iic_config_struct);
    iic_master[i].init_flag = i2c_driver_install(iic_master[i].port, iic_config_struct.mode, I2C_MASTER_TX_BUF_DISABLE, I2C_MASTER_RX_BUF_DISABLE, 0);

    if (iic_master[i].init_flag != ESP_OK)
    {
        while (1)
        {
            printf("%s, ret: %d", __func__, iic_master[i].init_flag);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    return iic_master[i];
}

esp_err_t i2c_transfer(i2c_obj_t* self, uint16_t addr, size_t n, i2c_buf_t* bufs, unsigned int flags)
{
    int data_len = 0;
    esp_err_t ret = ESP_FAIL;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    if (flags & I2C_FLAG_WRITE)
    {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, addr << 1, ACK_CHECK_EN);
        i2c_master_write(cmd, bufs->buf, bufs->len, ACK_CHECK_EN);
        data_len += bufs->len;
        --n;
        ++bufs;
    }
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 | (flags & I2C_FLAG_READ), ACK_CHECK_EN);

    for (; n--; ++bufs)
    {
        if (flags & I2C_FLAG_READ)
        {
            i2c_master_read(cmd, bufs->buf, bufs->len, n == 0 ? I2C_MASTER_LAST_NACK : I2C_MASTER_ACK);
        }
        else
        {
            if (bufs->len != 0)
            {
                i2c_master_write(cmd, bufs->buf, bufs->len, ACK_CHECK_EN);
            }
        }
        data_len += bufs->len;
    }

    if (flags & I2C_FLAG_STOP)
    {
        i2c_master_stop(cmd);
    }
    ret = i2c_master_cmd_begin(self->port, cmd, 100 * (1 + data_len) / portTICK_PERIOD_MS);
    
    i2c_cmd_link_delete(cmd);

    return ret;
}