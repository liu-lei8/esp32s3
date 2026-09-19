#pragma once /* tud_flash.h */

#include "tinyusb.h"
#include "tinyusb_msc.h"
#include "tinyusb_default_config.h"
#include "esp_partition.h"
#include "class/msc/msc.h"

/* SPI Flash 的 storage 分区 + wear levelling 磨损均衡 + FATFS 文件系统 + TinyUSB MSC 类
   = 电脑识别到的 U 盘*/

#define BASE_POINT  "/1:"       /* 挂载分区的基本路径 */
#define EPNUM_MSC   1           /* endpoint num端点号*/
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

enum {
    ITF_NUM_MSC = 0,        /* interface*/
    ITF_NUM_TOTAL
};

enum {
    EDPT_CTRL_OUT = 0X00,       /* 并没有使用 */
    EDPT_CTRL_IN  = 0X80,

    EDPT_MSC_OUT  = EPNUM_MSC,          /* 主机发给设备的端点地址 */
    EDPT_MSC_IN   = 0x80 | EPNUM_MSC,   /* 设备发给主机的端点地址，0x81 */
};

void tud_usb_flash(void);   /* flash模拟U盘初始化 */