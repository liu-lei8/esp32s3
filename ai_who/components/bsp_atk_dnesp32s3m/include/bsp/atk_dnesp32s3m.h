#pragma once

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "bsp/display.h"

#define BSP_BOARD_ATK_DNESP32S3M

/* ========== I2C（XL9555 使用） ========== */
#define BSP_I2C_NUM     I2C_NUM_0
#define BSP_I2C_SCL     (GPIO_NUM_9)
#define BSP_I2C_SDA     (GPIO_NUM_8)

/* ========== LCD (ST7789V, SPI) ========== */
#define BSP_LCD_SPI_NUM            (SPI2_HOST)
#define BSP_LCD_PIXEL_CLOCK_HZ     (40 * 1000 * 1000)
#define BSP_LCD_MOSI               (GPIO_NUM_11)
#define BSP_LCD_CLK                (GPIO_NUM_12)
#define BSP_LCD_DC                 (GPIO_NUM_40)   // 对应你代码里的 LCD_NUM_WR
#define BSP_LCD_CS                 (GPIO_NUM_21)
// RST 和背光通过 XL9555 控制，不是 GPIO
#define BSP_LCD_RST                (GPIO_NUM_NC)
#define BSP_LCD_BACKLIGHT          (GPIO_NUM_NC)

/* ========== 摄像头 OV5640 (DVP) ========== */
#define BSP_CAMERA_GPIO_XCLK  (GPIO_NUM_46)      // 尝试值，见下方说明
#define BSP_CAMERA_RST        (GPIO_NUM_NC)      // 通过 XL9555 控制
#define BSP_CAMERA_PWDN       (GPIO_NUM_NC)      // 通过 XL9555 控制
#define BSP_CAMERA_PCLK       (GPIO_NUM_45)
#define BSP_CAMERA_VSYNC      (GPIO_NUM_47)
#define BSP_CAMERA_HSYNC      (GPIO_NUM_48)
#define BSP_CAMERA_D0         (GPIO_NUM_4)
#define BSP_CAMERA_D1         (GPIO_NUM_5)
#define BSP_CAMERA_D2         (GPIO_NUM_6)
#define BSP_CAMERA_D3         (GPIO_NUM_7)
#define BSP_CAMERA_D4         (GPIO_NUM_15)
#define BSP_CAMERA_D5         (GPIO_NUM_16)
#define BSP_CAMERA_D6         (GPIO_NUM_17)
#define BSP_CAMERA_D7         (GPIO_NUM_18)
#define BSP_CAMERA_SIOD       (GPIO_NUM_39)
#define BSP_CAMERA_SIOC       (GPIO_NUM_38)
#define BSP_CAMERA_XCLK_CLOCK_MHZ  (24)

/* ========== XL9555 的 IO 定义 ========== */
#define ATK_OV_PWDN_IO       0x0010  /* P04，摄像头掉电 */
#define ATK_OV_RESET_IO      0x0020  /* P05，摄像头复位 */
#define ATK_SLCD_RST_IO      0x0400  /* P12，LCD 复位 */
#define ATK_SLCD_PWR_IO      0x0800  /* P13，LCD 背光/电源 */

/* ========== C++ 兼容包裹 ========== */
#ifdef __cplusplus
extern "C" {
#endif

/* ========== 摄像头配置结构 ========== */
typedef struct {
    uint8_t dummy;
} bsp_camera_cfg_t;

esp_err_t bsp_camera_start(const bsp_camera_cfg_t *cfg);

/* ========== I2C 接口 ========== */
esp_err_t bsp_i2c_init(void);
esp_err_t bsp_i2c_deinit(void);
i2c_master_bus_handle_t bsp_i2c_get_handle(void);

/* ========== XL9555 接口（供 BSP 内部使用） ========== */
esp_err_t xl9555_init_bsp(void);
esp_err_t xl9555_pin_write_bsp(uint16_t pin, bool level);

#ifdef __cplusplus
}
#endif