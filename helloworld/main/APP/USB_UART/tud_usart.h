#pragma once /* tud_usart.c */
/* 注意：在menuconfig中要打开这个配置：Component config → TinyUSB → CDC*/

#include "tinyusb.h"
#include "tinyusb_cdc_acm.h"   /* Communication Device Class Abstract control Model*/
#include "tinyusb_default_config.h"
#include "esp_log.h"

void tud_usb_usart(void);