#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define USART_UX             UART_NUM_0
#define USART_TX_GPIO_PIN   GPIO_NUM_43
#define USART_RX_GPIO_PIN   GPIO_NUM_44

#define RX_BUF_SIZE         1024    /*环形缓冲区大小*/

void usart_init(uint32_t baudrate);