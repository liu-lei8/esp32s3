#include "xl9555.h"

i2c_obj_t xl9555_i2c_master;

void xl9555_init(i2c_obj_t self)
{
    uint8_t r_data[2];

    if (self.init_flag == ESP_FAIL)
    {
        /* iic_init 返回已初始化的 i2c_obj_t，要使用其返回值 */
        xl9555_i2c_master = iic_init(self.port);
    }
    else
    {
        xl9555_i2c_master = self;
    }

    gpio_config_t gpio_init_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ull << XL9555_INT_IO,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&gpio_init_config);

    xl9555_read_byte(r_data, 2);    /*上电先读取一次清除中断标志*/
    xl9555_ioconfig(0xF003);
    xl9555_pin_write(BEEP_IO, 1);
    xl9555_pin_write(SPK_EN_IO, 1);
}

esp_err_t xl9555_write_byte(uint8_t reg, uint8_t* data, size_t len)
{
    i2c_buf_t bufs[2] = {
        {.len = 1, .buf = &reg},
        {.len = len, .buf = data},
    };

    return i2c_transfer(&xl9555_i2c_master, XL9555_ADDR, 2, bufs, I2C_FLAG_STOP);
}

esp_err_t xl9555_pin_write(uint16_t pin, bool level)
{
    uint8_t memaddr_buf[1] = {XL9555_OUTPUT_PORT0_REG};
    uint8_t data[2];
    i2c_buf_t bufs[2] = {
        {.len = 1, .buf = memaddr_buf},
        {.len = 2, .buf = data},
    };
    esp_err_t err;

    err = i2c_transfer(&xl9555_i2c_master, XL9555_ADDR, 2, bufs, I2C_FLAG_READ | I2C_FLAG_WRITE | I2C_FLAG_STOP);
    if (err != ESP_OK)
    {
        return ESP_FAIL;
    }

    uint16_t port = (uint16_t)(data[1] << 8) | data[0];
    
    if (level)
    {
        port |= pin;
    }
    else
    {
        port &= ~pin;
    }

    data[0] = (uint8_t)(port & 0xFF);
    data[1] = (uint8_t)((port >> 8) & 0xFF);

    err = xl9555_write_byte(*memaddr_buf, data, 2);
    return err == ESP_OK ? ESP_OK : ESP_FAIL;

}

esp_err_t xl9555_read_byte(uint8_t* data, size_t len)
{
    uint8_t memaddr_buf[1];
    memaddr_buf[0] = XL9555_INPUT_PORT0_REG;

    i2c_buf_t bufs[2] = {
        {.len = 1, .buf = memaddr_buf},
        {.len = len, .buf = data},
    };

    return i2c_transfer(&xl9555_i2c_master, XL9555_ADDR, 2, bufs, I2C_FLAG_WRITE | I2C_FLAG_READ | I2C_FLAG_STOP);
}

int xl9555_pin_read(uint16_t pin)
{
    uint16_t ret;
    uint8_t r_data[2];
    xl9555_read_byte(r_data, 2);
    ret = r_data[1] << 8 | r_data[0];

    return (ret & pin) ? 1 : 0;
}

uint16_t xl9555_ioconfig(uint16_t config_value)
{
    uint8_t data[2];
    esp_err_t err;
    int retry = 3;

    data[0] = (uint8_t)(0xFF & config_value);
    data[1] = (uint8_t)(0xFF & (config_value >> 8));

    do
    {
        err = xl9555_write_byte(XL9555_CONFIG_PORT0_REG, data, 2);
        if (err != ESP_OK)
        {
            retry--;
            vTaskDelay(100);
            ESP_LOGE("IIC", "%s configure %X failed, ret: %d", __func__, config_value, err);
            
            if (retry <= 0)
            {
                vTaskDelay(5000);
                esp_restart();
            }
        }
        else{
            break;
        }
    } while (retry);
    
    return config_value;
}

uint8_t xl9555_key_scan(uint8_t mode)
{
    uint8_t keyval = 0;
    static uint8_t boot_release = 1;   /*按键松开标志*/

    if (mode == 1)
    {
        boot_release = 1;
    }
    
    if (boot_release == 1 && (KEY0 == 0 || KEY1 == 0 || KEY2 == 0 || KEY3 == 0))
    {
        if (KEY0 == 0)
        {
            keyval = KEY0_PRES;
            boot_release = 0;
        }
        else if (KEY1 == 0)
        {
            keyval = KEY1_PRES;
            boot_release = 0;
        }
        else if (KEY2 == 0)
        {
            keyval = KEY2_PRES;
            boot_release = 0;
        }
        else if (KEY3 == 0)
        {
            keyval = KEY3_PRES;
            boot_release = 0;
        }
    }
    else if (KEY0 == 1 && KEY1 == 1 && KEY2 == 1 && KEY3 == 1)
    {
        boot_release = 1;
    }

    return keyval;
}