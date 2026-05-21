#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define DHT11_DQ_GPIO_PIN   GPIO_NUM_3

typedef enum{
    DHT11_PIN_RESET = 0u,
    DHT11_PIN_SET,
}DHT11_GPIO_PinState;

#define DHT11_DQ_IN     gpio_get_level(DHT11_DQ_GPIO_PIN)

#define DHT11_DQ_OUT(x) do{x ? gpio_set_level(DHT11_DQ_GPIO_PIN, DHT11_PIN_SET) : \
                               gpio_set_level(DHT11_DQ_GPIO_PIN, DHT11_PIN_RESET);  \
                            }while(0)

/**
 * @brief 初始化DHT11，返回0成功，1失败
 */
uint8_t dht11_init(void);
/**
 * @brief 从DHT11读取一次数据
 * @note 实际读出的小数点后面的数据都是0
 * @param temp: 温度值（范围：-20~60℃）
 * @param humi: 湿度值（范围：5％~95％）
 * @return 0:正常，1：失败
 */
uint8_t dht11_read_data(float* temp, float* humi);