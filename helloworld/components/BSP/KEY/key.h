#ifndef _KEY_H_
#define _KEY_H_

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define KEY_GPIO_PIN    GPIO_NUM_0
#define BOOT            gpio_get_level(KEY_GPIO_PIN)
#define BOOT_PRES       1

void key_init(void);
uint8_t key_scan(uint8_t mode);

#endif