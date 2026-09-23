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

/* WIFI连接初始化 */
void wifi_sta_init(void);