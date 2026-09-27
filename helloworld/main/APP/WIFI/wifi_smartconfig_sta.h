#pragma once /* wifi_smartconfig_sta.h */

#include "esp_wifi.h"
#include "esp_smartconfig.h"
#include "esp_event.h"
#include "lcd.h"

#define CONNECTED_BIT       BIT0
#define ESPTOUCH_DONE_BIT   BIT1

/* wifi一键配网 */
void wifi_smartconfig_sta(void);