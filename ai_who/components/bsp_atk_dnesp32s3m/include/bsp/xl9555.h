#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

#define XL9555_ADDR             0x20
#define XL9555_OUTPUT_PORT0_REG 0x02
#define XL9555_CONFIG_PORT0_REG 0x06

// XL9555 上的摄像头控制引脚
#define ATK_OV_PWDN_IO       0x0010  /* P04 */
#define ATK_OV_RESET_IO      0x0020  /* P05 */
// LCD 控制引脚
#define ATK_SLCD_RST_IO      0x0400  /* P12 */
#define ATK_SLCD_PWR_IO      0x0800  /* P13 */

esp_err_t xl9555_init_bsp(void);
esp_err_t xl9555_pin_write_bsp(uint16_t pin, bool level);