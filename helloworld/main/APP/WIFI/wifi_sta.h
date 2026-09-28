#pragma once        /* wifi_sta.h */

#include "esp_wifi.h"
#include "esp_event.h"
#include "lcd.h"

#define DEFAULT_SSID    "OnePlus 7T"
#define DEFAULT_PWD     "88888888"

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

#define WIFICONFIG()  {                             \
    .sta = {                                        \
        .ssid = DEFAULT_SSID,                       \
        .password = DEFAULT_PWD,                    \
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,   \
    },                                              \
}

typedef struct _network_connect_info_t
{
    uint8_t connect_state;      /* 网络连接状态 */
    char ip_buf[100];           /* 分配的IP地址 */
    char mac_buf[100];          /* 子网掩码 */
    void (*fun)(uint8_t x);     /* 函数指针 */
}network_connect_info;

extern network_connect_info network_connet;


/* WIFI连接初始化 */
void wifi_sta_init(void);