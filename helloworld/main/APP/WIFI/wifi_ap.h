#pragma once    /* wifi_ap.h */

#include "esp_wifi.h"
#include "lcd.h"
#include "lwip/inet.h"

#define EXAMPLE_ESP_WIFI_SSID   "ESP32-S3"
#define EXAMPLE_ESP_WIFI_PWD    "88888888"
#define EXAMPLE_MAX_STA_CONN    5
#define MAC2STR(a)  (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR  "%02x:%02x:%02x:%02x:%02x:%02x"

void wifi_softap_init(void);