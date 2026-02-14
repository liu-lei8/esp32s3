#include "soft_iic.h"

#define TAG "SOFT_IIC"

//私有函数声明
static void soft_iic_gpio_init(soft_iic_t* iic);
static void soft_iic_bus_init(soft_iic_t* iic);

soft_iic_t* soft_iic_init(gpio_num_t scl_pin, gpio_num_t sda_pin, uint32_t frequency)
{
    soft_iic_t* iic = (soft_iic_t*)malloc(sizeof(soft_iic_t));
    if (iic ==NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for soft_iic_t");
        return NULL;
    }

    iic->scl_pin = scl_pin;
    iic->sda_pin = sda_pin;
    iic->frequency = frequency;

    /*计算延迟时间，每个周期需要4次延时（高电平，低电平，高变低，低变高）*/
    /*一个周期的时间是1/frequency,转化为微秒就是1000000/frequency*/
    /*把一个周期的时间分成四等分，如果frequency = 100khz，那么此时延迟时间就为2.5us,取整就是2us*/
    iic->delay_us = 1000000 / (frequency * 4);
    if (iic->delay_us == 0)
    {
        iic->delay_us = 1; /*最小延迟1us*/
    }

    soft_iic_gpio_init(iic);
    soft_iic_bus_init(iic);

    iic->initialized = true;

    ESP_LOGI(TAG, "Soft IIC initialized: SCL=%d, SDA=%d, Freq=%dHz, Delay=%dus", iic->scl_pin, iic->sda_pin, iic->frequency, iic->delay_us);

    return iic;
}

void soft_iic_deinit(soft_iic_t* iic)
{
    if (iic != NULL)
    {
        /*释放总线*/
        soft_iic_stop(iic);
        free(iic);
    }
}

static void soft_iic_gpio_init(soft_iic_t* iic)
{
    gpio_config_t scl_cfg = {
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,        /*open-drain开漏输入输出模式*/
        .pin_bit_mask = 1ull << iic->scl_pin,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&scl_cfg);

    gpio_config_t sda_cfg = {
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pin_bit_mask = 1ull << iic->sda_pin,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&sda_cfg);

    /*初始化为高电平*/
    soft_iic_set_scl(iic, true);
    soft_iic_set_sda(iic, true);
}

static void soft_iic_bus_init(soft_iic_t* iic)
{
    soft_iic_set_scl(iic, true);
    soft_iic_set_sda(iic, true);
    soft_iic_delay(iic);
}

void soft_iic_set_scl(soft_iic_t* iic, bool level)
{
    gpio_set_level(iic->scl_pin, level ? 1 : 0);
}

void soft_iic_set_sda(soft_iic_t* iic, bool level)
{
    gpio_set_level(iic->sda_pin, level ? 1 : 0);
}

uint32_t soft_iic_read_sda(soft_iic_t* iic)
{
    /*将iic数据线临时设置为输入模式*/
    //gpio_set_direction(iic->sda_pin, GPIO_MODE_INPUT);
    uint32_t level = gpio_get_level(iic->sda_pin);      /*频繁的切换方向可能导致SDA引脚状态丢失*/
    /*恢复为开漏输出模式*/
    //gpio_set_direction(iic->sda_pin, GPIO_MODE_OUTPUT_OD);

    return level;
}

/*此处使用微秒级延迟第二种方案*/
void soft_iic_delay(soft_iic_t* iic)
{
    if (iic->delay_us > 0)
    {
        esp_rom_delay_us(iic->delay_us);
    }
}

void soft_iic_start(soft_iic_t* iic)
{
    /*先把数据线拉高，再把时钟线拉高，否则在发送读寄存器地址后再发送起始信号会导致误发了停止信号*/
    soft_iic_set_sda(iic, true);
    soft_iic_set_scl(iic, true);
    soft_iic_delay(iic);

    soft_iic_set_sda(iic, false);
    soft_iic_delay(iic);

    soft_iic_set_scl(iic, false);
    soft_iic_delay(iic);
}

void soft_iic_stop(soft_iic_t* iic)
{
    soft_iic_set_scl(iic, false);
    soft_iic_set_sda(iic, false);
    soft_iic_delay(iic);

    soft_iic_set_scl(iic, true);
    soft_iic_delay(iic);

    soft_iic_set_sda(iic, true);
    soft_iic_delay(iic);
}

uint8_t soft_iic_send_byte(soft_iic_t* iic, uint8_t data)
{
    bool level;
    uint8_t ack;

    for (size_t i = 0; i < 8; i++)
    {
        level = (data & 0x80) ? 1 : 0;        /*先发送高位*/
        soft_iic_set_sda(iic, level);       /*准备要发送的数据*/
        soft_iic_delay(iic);

        soft_iic_set_scl(iic, true);        /*发送数据*/
        soft_iic_delay(iic);

        soft_iic_set_scl(iic, false);       /*要求数据线准备数据*/
        soft_iic_delay(iic);

        data <<= 1;
    }

    soft_iic_set_sda(iic, true);            /*释放数据线，主机准备读取从机应答*/
    soft_iic_delay(iic);

    soft_iic_set_scl(iic, true);            /*SCL拉高，从机发送应答信号*/
    soft_iic_delay(iic);

    ack = (soft_iic_read_sda(iic) == 0) ? 0 : 1;   /*读到数据线为0表示从机应答*/

    soft_iic_set_scl(iic, false);           /*SCL拉低，准备下一次数据传输*/
    soft_iic_delay(iic);

    return ack;     /*0为应答，1非应答*/
}

uint8_t soft_iic_receive_byte(soft_iic_t* iic, bool ack)
{
    uint8_t r_data = 0;

    soft_iic_set_sda(iic, true);    /*释放数据线，准备读取*/
    soft_iic_delay(iic);

    for (size_t i = 0; i < 8; i++)
    {
        r_data <<= 1;

        soft_iic_set_scl(iic, true);
        soft_iic_delay(iic);
        r_data |= soft_iic_read_sda(iic);

        soft_iic_set_scl(iic, false);
        soft_iic_delay(iic);
    }

    soft_iic_set_sda(iic, ack);     /*主机发送应答或非应答信号，ack = 0应答，1非应答*/
    soft_iic_delay(iic);

    soft_iic_set_scl(iic, true);    /*传输应答信号给从机*/
    soft_iic_delay(iic);

    soft_iic_set_scl(iic, false);
    soft_iic_delay(iic);

    return r_data;
}

esp_err_t soft_iic_write(soft_iic_t* iic, uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t len)
{
    /*起始信号*/
    soft_iic_start(iic);

    /*发送设备地址(写模式)*/
    if (soft_iic_send_byte(iic, dev_addr << 1))
    {
        soft_iic_stop(iic);
        ESP_LOGE(TAG, "Device not ACK after sending the writing device address");
        return ESP_FAIL;
    }

    /*发送寄存器地址*/
    if (soft_iic_send_byte(iic, reg_addr))
    {
        soft_iic_stop(iic);
        ESP_LOGE(TAG, "Device not ACK after sending the writing register address");
        return ESP_FAIL;
    }

    for (size_t i = 0; i < len; i++)
    {
        if (soft_iic_send_byte(iic, data[i]))
        {
            soft_iic_stop(iic);
            ESP_LOGE(TAG, "Device not ACK after sending data");
            return ESP_FAIL;
        }
    }

    soft_iic_stop(iic);

    return ESP_OK;
}

esp_err_t soft_iic_read(soft_iic_t* iic, uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t len)
{
    if (iic == NULL || !iic->initialized)
    {
        return ESP_FAIL;
    }

    soft_iic_start(iic);

    /*发送设备地址（写模式）*/
    if (soft_iic_send_byte(iic, (dev_addr << 1 | 0x00)))
    {
        soft_iic_stop(iic);
        ESP_LOGE(TAG, "Device not ACK after sending the writing device address for reading data");
        return ESP_FAIL;
    }

    /*发送寄存器地址*/
    if (soft_iic_send_byte(iic, reg_addr))
    {
        soft_iic_stop(iic);
        ESP_LOGE(TAG, "Device not ACK after sending the reading register address");
        return ESP_FAIL;
    }

    soft_iic_start(iic);

    /*再发送设备地址（读模式）*/
    if (soft_iic_send_byte(iic, (dev_addr << 1 | 0x01)))
    {
        soft_iic_stop(iic);
        ESP_LOGE(TAG, "Device not ACK after sending the reading device address");
        return ESP_FAIL;
    }

    for (size_t i = 0; i < len; ++i)
    {
        data[i] = soft_iic_receive_byte(iic, (i == len - 1) ? 1 : 0); /*读到最后一个数据发送非应答信号*/
    }

    soft_iic_stop(iic);

    return ESP_OK;
}

esp_err_t soft_iic_check_device(soft_iic_t* iic, uint8_t dev_addr)
{
    if (iic == NULL || !iic->initialized)
    {
        return ESP_FAIL;
    }

    soft_iic_start(iic);
    uint8_t ack = soft_iic_send_byte(iic, dev_addr << 1);
    soft_iic_stop(iic);

    return (ack == 0) ? ESP_OK : ESP_FAIL;
}
