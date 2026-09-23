#pragma once "wifi_scan.h"

#include "esp_wifi.h"
#include <string.h>
#include "lcd.h"

#define DEFAULT_SCAN_LIST_SIZE  12

void wifi_scan(void);