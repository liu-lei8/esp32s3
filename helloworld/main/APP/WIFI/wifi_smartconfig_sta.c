#include "wifi_smartconfig_sta.h"

static EventGroupHandle_t s_wifi_event_group;
static const char* TAG = "smartconfig_example";
static char lcd_buf[100] = {0};

static void smartconfig_task(void* arg);

/**
 * @brief       WIFI链接回调函数
 * @param       arg:传入网卡控制块
 * @param       event_base:WIFI事件
 * @param       event_id:事件ID
 * @param       event_data:事件数据
 * @retval      无
 */
static void event_hanlder(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        xTaskCreatePinnedToCore(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL, 0);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE)
    {
        ESP_LOGI(TAG, "Scan done !");
        lcd_show_string(30, 110, "In the distribution network........", BLUE ,WHITE, 16, 1);
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL)
    {
        ESP_LOGI(TAG, "Found channel");
    }
    /* 获取到SSID和密码，表示配网成功 */
    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
    {
        ESP_LOGI(TAG, "Got SSID and password");
        
        smartconfig_event_got_ssid_pswd_t* evt = (smartconfig_event_got_ssid_pswd_t*)event_data;
        wifi_config_t wifi_cfg;
        uint8_t ssid[33] = {0};
        uint8_t password[65] = {0};
        uint8_t rvd_data[33] = {0};     /* Reserved Data “保留数据” 或 “自定义数据”*/

        bzero(&wifi_cfg, sizeof(wifi_config_t));
        memcpy(wifi_cfg.sta.ssid, evt->ssid, sizeof(wifi_cfg.sta.ssid));
        memcpy(wifi_cfg.sta.password, evt->password, sizeof(wifi_cfg.sta.password));
        wifi_cfg.sta.bssid_set = evt->bssid_set;

        if (wifi_cfg.sta.bssid_set == true) /* 硬件标识设置为1，那么就要连接对应的ssid名的对应设备 */
        {
            memcpy(wifi_cfg.sta.bssid, evt->bssid, sizeof(wifi_cfg.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);

        lcd_fill(0, 110, 240, 280, WHITE);
        sprintf(lcd_buf, "%s", ssid);
        lcd_show_string(30, 110, lcd_buf, BLUE, WHITE, 16, 1);
        sprintf(lcd_buf, "%s", password);
        lcd_show_string(30, 130, lcd_buf, BLUE, WHITE, 16, 1);
        lcd_show_string(30, 150, "Successful distribution network", BLUE, WHITE, 16, 1);

        /* 手机APP ESPTOUCH软件使用ESPTOUCH V2模式，会执行以下代码 */
        if (evt->type == SC_TYPE_ESPTOUCH_V2)
        {
            ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
            ESP_LOGI(TAG, "RVD_DATA:");

            for (int i = 0; i < 33; i++)
            {
                printf("%02x ", rvd_data[i]);
            }

            putchar('\n');
        }

        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
        esp_wifi_connect();
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE)
    {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

void wifi_smartconfig_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_hanlder, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_hanlder, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_hanlder, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/**
 * @brief       一键配网回调函数，使用V2模式
 * @param       parm:传入的形参(未使用)
 * @retval      无
 */
static void smartconfig_task(void* arg)
{
    EventBits_t uxBits;
    /* 设置配网协议,V2模式 */
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2));
    /* 设置配网参数 */
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    cfg.esp_touch_v2_enable_crypt = true;       /* 启用加密 */
    cfg.esp_touch_v2_key = "1234567890123456";  /* 16字节的AES秘钥 */
    /* 开始配网 */
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));

    while (1)
    {
        /* 获取事件 */
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);

        if (uxBits == CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "WiFi connected to ap");
        }

        if (uxBits == ESPTOUCH_DONE_BIT)
        {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}


