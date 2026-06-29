#ifndef _KEY_H_
#define _KEY_H_

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define KEY_GPIO_PIN    GPIO_NUM_0
#define BOOT            gpio_get_level(KEY_GPIO_PIN)
#define BOOT_PRES       1

void key_init(void);
/**
 * @brief ESP32S3开发板自带的key0，对其进行按键扫描
 * @param mode:0不支持连续按键，1支持连续按键
 * @return 0按键未按下，1按键按下
 */
uint8_t key_scan(uint8_t mode);

#endif