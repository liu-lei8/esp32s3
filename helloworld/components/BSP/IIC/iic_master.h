#pragma once

#include "driver/i2c_master.h"
#include "esp_log.h"


typedef struct {
    i2c_port_t port;
    gpio_num_t scl;
    gpio_num_t sda;
    i2c_master_bus_handle_t bus_handle;     /*新增总线句柄*/
    esp_err_t init_flag;
}iic_obj_t;

typedef struct {
    uint8_t* buf;
    size_t len;
}iic_buf_t;

iic_obj_t iic_init_new(i2c_port_t port);
esp_err_t iic_transfer_new(iic_obj_t* self, uint16_t addr, size_t n, iic_buf_t* bufs, uint8_t flags);