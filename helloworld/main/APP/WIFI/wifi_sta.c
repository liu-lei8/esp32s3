#include "wifi_sta.h"

static EventGroupHandle_t wifi_event;
static const char* TAG = "wifi_sta";
char lcd_buf[100] = {0};

/**
 * @brief 连接显示到lcd上
 * @param flag:2->连接成功，1->连接失败，0->再连接中
 * @retval 无
 */
static void connect_display(uint8_t flag)
{
    if (flag == 2)
    {
        lcd_fill(0, 110, 240, 280, WHITE);
        sprintf(lcd_buf, "ssid: %s", DEFAULT_SSID);
        lcd_show_string(30, 110, lcd_buf, BLUE, WHITE, 16, 1);
        sprintf(lcd_buf, "pwd: %s", DEFAULT_PWD);
        lcd_show_string(30, 130, lcd_buf, BLUE, WHITE, 16, 1);
    }
    else if (flag == 1)
    {
        lcd_show_string(30, 110, "wifi connecting fail", BLUE, WHITE, 16, 1);
    }
    else
    {
        lcd_show_string(30, 110, "wifi connecting.......", BLUE, WHITE, 16, 1);
    }
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
        connect_display(0);
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        connect_display(2);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "connect to the AP fail");

        if (s_retry_num < 20)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(wifi_event, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event, WIFI_CONNECTED_BIT);
    }
}

void wifi_sta_init(void)
{
    static esp_netif_t* sta_netif = NULL;
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
        connect_display(1);
        ESP_LOGI(TAG, "fail to connect SSID:%s password:%s", DEFAULT_SSID, DEFAULT_PWD);
    }
    else
    {
        ESP_LOGI(TAG, "unexpected event");
    }
}


