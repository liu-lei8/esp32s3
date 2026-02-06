#include "xl9555.h"

 i2c_obj_t xl9555_i2c_master;
 TaskHandle_t key_task_handle = NULL;


static void IRAM_ATTR xl9555_intr_callback(void *arg)
{
    BaseType_t xHigherPeriorityTaskWoken = pdFALSE;
    TaskHandle_t* key_task = (TaskHandle_t*) arg;

    vTaskNotifyGiveFromISR(*key_task, &xHigherPeriorityTaskWoken);

    if (xHigherPeriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

static void xl9555_key_scan_task(void* arg)
{
    uint8_t key;
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        key = xl9555_key_scan(0);
        switch (key)
        {
            case KEY0_PRES:
            {
                printf("key0 has been press\n");
                xl9555_pin_write(BEEP_IO, 0);
                break;
            }
            case KEY1_PRES:
            {
                printf("key1 has been press\n");
                xl9555_pin_write(BEEP_IO, 1);
                break;
            }
            case KEY2_PRES:
            {
                printf("key2 has been press\n");
                LED(0);
                break;
            }
            case KEY3_PRES:
            {
                printf("key3 has been press\n");
                LED(1);
                break;
            }
            default:break;
        }
    }
}

void xl9555_init(i2c_obj_t self)
{
    uint8_t r_data[2];

    if (self.init_flag != ESP_OK)
    {
        xl9555_i2c_master = iic_init(self.port);
    }
    else
    {
        xl9555_i2c_master = self;
    }

    gpio_config_t xl9555_int_cfg = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ull << XL9555_INT_IO,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&xl9555_int_cfg);

    xTaskCreatePinnedToCore(xl9555_key_scan_task, "xl9555_key_scan_task", 2048, NULL, 5, &key_task_handle, 1);

    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    gpio_isr_handler_add(XL9555_INT_IO, xl9555_intr_callback, &key_task_handle);
    gpio_intr_enable(XL9555_INT_IO);

    xl9555_read_byte(r_data, 2);    /*上电先读取一次清除中断标志*/
    // 1. 清除任何可能存在的假中断标志
    // 2. 让INT引脚恢复高电平
    // 3. 重置内部的状态比较器
    xl9555_ioconfig(0xF003);
    xl9555_pin_write(BEEP_IO, 1);

}

esp_err_t xl9555_write_byte(uint8_t reg, uint8_t* data, size_t len)
{
    i2c_buf_t bufs[2] = {
        {.buf = &reg, .len = 1},
        {.buf = data, .len = len}
    };

    return iic_transfer(&xl9555_i2c_master, XL9555_ADDR, 2, bufs, IIC_FLAG_STOP);
}

esp_err_t xl9555_pin_write(uint16_t xl9555_pin, bool level)
{
    uint8_t memaddr_buf[1] = {XL9555_OUTPUT_PORT0_REG};
    uint8_t data[2];

    i2c_buf_t bufs[2] = {
        {.buf = memaddr_buf, .len = 1},
        {.buf = data, .len = 2}
    };
    esp_err_t err = iic_transfer(&xl9555_i2c_master, XL9555_ADDR, 2, bufs, IIC_FLAG_READ | IIC_FLAG_STOP | IIC_FLAG_WRITE);

    uint16_t ret = data[1] << 8 | data[0];

    if (level)
    {
        ret |= xl9555_pin;
    }
    else
    {
        ret &= ~xl9555_pin;
    }

    data[0] = (uint8_t)(ret & 0xFF);
    data[1] = (uint8_t)((ret >> 8) & 0xFF);

    return xl9555_write_byte(memaddr_buf[0], data, 2);
}

esp_err_t xl9555_read_byte(uint8_t* data, size_t len)
{
    uint8_t memaddr_buf[1] = {XL9555_INPUT_PORT0_REG};

    i2c_buf_t bufs[2] = {
        {.buf = memaddr_buf, .len = 1},
        {.buf = data, .len = len}
    };

    return iic_transfer(&xl9555_i2c_master, XL9555_ADDR, 2, bufs, IIC_FLAG_READ | IIC_FLAG_STOP | IIC_FLAG_WRITE);
}

bool xl9555_pin_read(uint16_t xl9555_pin)
{
    uint8_t data[2];
    xl9555_read_byte(data, 2);
    uint16_t ret = data[1] << 8 | data[0];
    
    return (ret & xl9555_pin) ? 1 : 0;
}

uint16_t xl9555_ioconfig(uint16_t ioconfig)
{
    uint8_t data[2];
    esp_err_t err;
    uint8_t retry = 3;

    data[0] = ioconfig & 0xFF;
    data[1] = (ioconfig >> 8) & 0xFF;

    do
    {
        err = xl9555_write_byte(XL9555_CONFIG_PORT0_REG, data, 2);
        if (err != ESP_OK)
        {
            ESP_LOGE("XL9555", "%s configure %#X failed, ret :%d", __func__, ioconfig, err);
            vTaskDelay(100);
            if (retry <= 0)
            {
                vTaskDelay(5000);
                esp_restart();
            }
        }
        else
        {
            break;
        }
    } while (--retry);
    
    return ioconfig;
}

uint8_t xl9555_key_scan(bool mode)
{
    static bool boot_release = 1;      /*按键释放标志*/

    if (mode)
    {
        boot_release = 1;
    }

    if (boot_release && (KEY0  == 0 || KEY1 == 0 || KEY2 == 0 || KEY3 == 0))
    {
        vTaskDelay(20);
        if (KEY0 == 0)
        {
            boot_release = 0;
            return KEY0_PRES;
        }
        else if (KEY1 == 0)
        {
            boot_release = 0;
            return KEY1_PRES;
        }
        else if (KEY2 == 0)
        {
            boot_release = 0;
            return KEY2_PRES;
        }
        else if (KEY3 == 0)
        {
            boot_release = 0;
            return KEY3_PRES;
        } 
    }
    else if (KEY0 == 1 && KEY1 == 1 && KEY2 == 1 && KEY3 == 1)
    {
        boot_release = 1;
    }

    return 0;
}