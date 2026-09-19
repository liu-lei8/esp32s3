#pragma once /* tud_sd.h */

#include "tinyusb.h"
#include "tinyusb_msc.h"
#include "tinyusb_default_config.h"
#include "esp_log.h"
#include "spi_sd.h"

#define BASE_PATH   "/0:"

void tud_usb_sd(void);      /* SD卡模拟U盘初始化 */