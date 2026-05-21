#include "dht11.h"

static void dht11_reset(void)
{
    DHT11_DQ_OUT(0);
    vTaskDelay(25);
    DHT11_DQ_OUT(1);
    esp_rom_delay_us(30);
}

/**
 * @brief 检测应答，返回0应答，1非应答
 */
static uint8_t dht11_check(void)
{
    uint8_t retry = 0;
    uint8_t rval = 0;

    while (DHT11_DQ_IN && retry < 100) /*等DHT11拉低电平，等100us，拉低约83us*/
    {
        retry++;
        esp_rom_delay_us(1);
    }
    if (retry >= 100)   /*等100us还没有拉低，说明有问题*/
    {
        rval = 1;
    }
    else
    {
        retry = 0;
        while (!DHT11_DQ_IN && retry < 100)     /*等DHT11拉高电平，等100us，拉高约87us*/
        {
            retry++;
            esp_rom_delay_us(1);
        }
        if (retry >= 100)
        {
            rval = 1;
        }
    }

    return rval;
}

uint8_t dht11_init(void)
{
    gpio_config_t dht11_gpio_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,      /*开漏输入输出模式*/
        .pin_bit_mask = 1 << DHT11_DQ_GPIO_PIN,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&dht11_gpio_cfg);

    dht11_reset();
    return dht11_check();
}

static uint8_t dht11_read_bit(void)
{
    uint8_t retry = 0;

    while (DHT11_DQ_IN && retry < 100)      /*等待拉低54us*/
    {
        retry++;
        esp_rom_delay_us(1);
    }

    retry = 0;
    while (!DHT11_DQ_IN && retry < 100)     /*等待拉高，逻辑0：23-27us，逻辑1：68-74us*/
    {
        retry++;
        esp_rom_delay_us(1);
    }

    esp_rom_delay_us(40);

    return DHT11_DQ_IN;
}

static uint8_t dht11_read_byte(void)
{
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        data <<= 1;
        data |= dht11_read_bit();
    }

    return data;
}

uint8_t dht11_read_data(float* temp, float* humi)
{
    uint8_t buf[5];

    dht11_reset();
    if (dht11_check() == 0)
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            buf[i] = dht11_read_byte();
        }
        if (buf[0] + buf[1] + buf[2] + buf[3] == buf[4])
        {
            *humi = buf[0] + (float)buf[1] / 1000;
            *temp = buf[2] + (float)buf[3] / 1000;
        }
    }
    else
    {
        return 1;
    }

    return 0;
}
