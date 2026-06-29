#pragma once    /*camera.h*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xl9555.h"
#include "esp_camera.h"
#include "lcd.h"


/*引脚配置*/
#define CAM_PIN_PWDN    GPIO_NUM_NC
#define CAM_PIN_RESET   GPIO_NUM_NC
#define CAM_PIN_VSYNC   GPIO_NUM_47
#define CAM_PIN_HREF    GPIO_NUM_48
#define CAM_PIN_PCLK    GPIO_NUM_45
#define CAM_PIN_XCLK    GPIO_NUM_NC
#define CAM_PIN_SIOD    GPIO_NUM_39     /*SCCB数据线*/
#define CAM_PIN_SIOC    GPIO_NUM_38     /*SCCB时钟线*/
#define CAM_PIN_D0      GPIO_NUM_4
#define CAM_PIN_D1      GPIO_NUM_5
#define CAM_PIN_D2      GPIO_NUM_6
#define CAM_PIN_D3      GPIO_NUM_7
#define CAM_PIN_D4      GPIO_NUM_15
#define CAM_PIN_D5      GPIO_NUM_16
#define CAM_PIN_D6      GPIO_NUM_17
#define CAM_PIN_D7      GPIO_NUM_18

#define CAM_PWDN(x)     do{ x ? xl9555_pin_write(OV_PWDN_IO, 1):    \
                                xl9555_pin_write(OV_PWDN_IO, 0);    \
                        }while(0)

#define CAM_RST(x)      do{ x ? xl9555_pin_write(OV_RESET_IO, 1):   \
                                xl9555_pin_write(OV_RESET_IO, 0);   \
                        }while(0)

extern camera_fb_t* fb;

esp_err_t camera_init(void);
/**
 * @brief 将摄像头模块的数据显示到spilcd上，像素240 * 240
 * @param x0y0: 起始坐标
 * @param data: RGB565像素数据
 */
void camera_show(uint16_t x0, uint16_t y0, uint8_t* data);
