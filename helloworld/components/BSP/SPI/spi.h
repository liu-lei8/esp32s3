#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include <string.h>


#define SPI_MOSI_GPIO_PIN       GPIO_NUM_11
#define SPI_CLK_GPIO_PIN        GPIO_NUM_12
#define SPI_MISO_GPIO_PIN       GPIO_NUM_13

void spi2_init(void);
void spi2_write_cmd(spi_device_handle_t handle, uint8_t cmd);
void spi2_write_data(spi_device_handle_t hanlde, const uint8_t* data, int len);
uint8_t spi2_transfer_byte(spi_device_handle_t handle, uint8_t data);
