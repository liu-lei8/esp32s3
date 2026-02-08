#include "iic_master.h"
#include "iic.h"

iic_obj_t iic_master[I2C_NUM_MAX];

iic_obj_t iic_init_new(i2c_port_t port)
{
    uint8_t i;
    if (port == I2C_NUM_0)
    {
        i = 0;
    }
    else
    {
        i = 1;
    }

    iic_master[i].port = port;
    iic_master[i].init_flag = ESP_FAIL;
    
    if (iic_master[i].port == I2C_NUM_0)
    {
        iic_master[i].scl = IIC0_SCL_GPIO_PIN;
        iic_master[i].sda = IIC0_SDA_GPIO_PIN;
    }
    else
    {
        iic_master[i].scl = IIC1_SCL_GPIO_PIN;
        iic_master[i].sda = IIC1_SDA_GPIO_PIN;
    }

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = true,
        .glitch_ignore_cnt = 7,
        .i2c_port = iic_master[i].port,
        .scl_io_num = iic_master[i].scl,
        .sda_io_num = iic_master[i].sda,
    };
    iic_master[i].init_flag = i2c_new_master_bus(&bus_cfg, &iic_master[i].bus_handle);
    
    if (iic_master[i].init_flag != ESP_OK)
    {
        while (1)
        {
            ESP_LOGE("IIC_MASTER", "%s, ret: %d", __func__, iic_master[i].init_flag);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    return iic_master[i];
}

esp_err_t iic_transfer_new(iic_obj_t* self, uint16_t addr, size_t n, iic_buf_t* bufs, uint8_t flags)
{
    esp_err_t ret = ESP_FAIL;
    i2c_master_dev_handle_t dev_handle = NULL;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = IIC_FREQ,
    };
    ret = i2c_master_bus_add_device(self->bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK)
    {
        return ret;
    }

    if (flags & IIC_FLAG_WRITE)
    {
        ret = i2c_master_transmit(dev_handle, bufs->buf, bufs->len, -1);
        if (ret != ESP_OK)
        {
            i2c_master_bus_rm_device(dev_handle);
            return ret;
        }
        ++bufs;
        --n;
    }

    for (; n--; ++bufs)
    {
        if (flags & IIC_FLAG_READ)
        {
            ret = i2c_master_receive(dev_handle, bufs->buf, bufs->len, -1);
        }
        else
        {
            ret = i2c_master_transmit(dev_handle, bufs->buf, bufs->len, -1);
        }

        if (ret != ESP_OK)
        {
            break;
        }
    }

    i2c_master_bus_rm_device(dev_handle);

    return ret;

}