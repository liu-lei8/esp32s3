#include "wifi_sta.h"

static EventGroupHandle_t wifi_event;
static const char* TAG = "wifi_sta";
char lcd_buf[100] = {0};

network_connect_info network_connet;

/**
 * @brief 连接显示到lcd上
 * @param flag:8->连接成功，4->再连接中，2->连接失败，1->获取到的热点AP的IP地址
 * @retval 无
 */
static void connect_display(uint8_t flag)
{
    if (flag & 0x08)
    {
        lcd_fill(0, 110, 240, 280, WHITE);
        sprintf(lcd_buf, "ssid: %s", DEFAULT_SSID);
        lcd_show_string(30, 110, lcd_buf, BLUE, WHITE, 16, 1);
        sprintf(lcd_buf, "pwd: %s", DEFAULT_PWD);
        lcd_show_string(30, 130, lcd_buf, BLUE, WHITE, 16, 1);
        lcd_show_string(30, 150, "KEY0:send data", MAGENTA, WHITE, 16, 1);
    }
    else if (flag & 0x04)
    {
        lcd_show_string(30, 110, "wifi connecting.......", BLUE, WHITE, 16, 1);
    }
    else if (flag & 0x02)
    {
        lcd_show_string(30, 110, "wifi connecting fail", BLUE, WHITE, 16, 1);
    }
    else if (flag & 0x01)
    {
        lcd_show_string(30, 170, network_connet.ip_buf, MAGENTA, WHITE, 16, 1);
    }

    network_connet.connect_state &= 0x00;
}

/**
 * @brief wifi连接回调函数
 * @param arg：传入网卡控制块
 * @param event_base: wifi事件
 * @param event_id: 事件id
 * @param event_data：事件数据
 * @retval 无
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    static int s_retry_num = 0;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        network_connet.connect_state |= 0x04;
        network_connet.fun(network_connet.connect_state);
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        network_connet.connect_state |= 0x08;
        network_connet.fun(network_connet.connect_state);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "connect to the AP fail");
        network_connet.connect_state |= 0x02;

        if (s_retry_num < 20)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(wifi_event, WIFI_FAIL_BIT);
            network_connet.fun(network_connet.connect_state);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        network_connet.connect_state |= 0x01;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;

        sprintf(network_connet.ip_buf, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
        network_connet.fun(network_connet.connect_state);
        xEventGroupSetBits(wifi_event, WIFI_CONNECTED_BIT);
    }
}

void wifi_sta_init(void)
{
    static esp_netif_t* sta_netif = NULL;
    network_connet.connect_state = 0x00;
    network_connet.fun = connect_display;

    wifi_event = xEventGroupCreate();       /* 创建一个事件标志组 */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_cfg = WIFICONFIG();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* 等待链接成功，获取IP */
    EventBits_t bits = xEventGroupWaitBits(wifi_event, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, false, false, portMAX_DELAY);

    /* 判断链接事件 */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connnected to ap SSID:%s password:%s", DEFAULT_SSID, DEFAULT_PWD);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "fail to connect SSID:%s password:%s", DEFAULT_SSID, DEFAULT_PWD);
    }
    else
    {
        ESP_LOGI(TAG, "unexpected event");
    }

    vEventGroupDelete(wifi_event);
}


