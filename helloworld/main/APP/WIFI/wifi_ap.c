#include "wifi_ap.h"

static const char* TAG = "wifi_ap";
static char lcd_buf[100] = {0};

/**
 * @brief wifi连接回调函数
 * @param arg：传入网卡控制块
 * @param event_base: wifi事件
 * @param event_id: 事件id
 * @param event_data：事件数据
 * @retval 无
 */
static void wifi_event_handler(void*arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    /* 设备连接 */
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        lcd_fill(0, 110, 240, 280, WHITE);
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
        /* Association Identifier: AID关联标识符，就是路由器提供给连接设备的识别编号 */
        ESP_LOGI(TAG, "station " MACSTR " join, AID = %d", MAC2STR(event->mac), event->aid);
        sprintf(lcd_buf, "MACSTR:"MACSTR, MAC2STR(event->mac));
        lcd_show_string(30, 110, lcd_buf, BLUE, WHITE, 16, 1);
        lcd_show_string(30, 130, "with device connection", BLUE, WHITE, 16, 1);
    }
    /* 设备断开 */
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID = %d", MAC2STR(event->mac), event->aid);
        lcd_fill(0, 110, 240, 280, WHITE);
        sprintf(lcd_buf, "Device disconnected:");
        lcd_show_string(30, 110, lcd_buf, BLUE, WHITE, 16, 1);
        sprintf(lcd_buf, MACSTR, MAC2STR(event->mac));
        lcd_show_string(30, 130, lcd_buf, BLUE, WHITE, 16, 1);
    }
}

void wifi_softap_init(void)
{
    static esp_netif_t* ap_netif = NULL;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    wifi_config_t wifi_cfg = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PWD,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    if (strlen(EXAMPLE_ESP_WIFI_PWD) == 0)          /* 没有密码就把认证模式设置成无需密码的认证模式 */
    {
        wifi_cfg.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);      /* 将热点ip地址从二进制转换为ASCII十进制地址 */
    ESP_LOGI(TAG, "set up softAP with ip:%s", ip_addr);

    ESP_LOGI(TAG, "wifi_softap_init finished. SSID:%s, PWD:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PWD);
    lcd_show_string(30, 110, "wifi connecting.........", BLUE, WHITE, 16, 1);
}

