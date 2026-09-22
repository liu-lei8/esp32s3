#include "wifi_scan.h"

static const char* TAG = "wifi_scan";

/**
 * @brief       打印身份认证模式
 * @param       authmode :身份验证模式
 * @retval      无
 */
static void print_auth_mode(int authmode)
{
    switch (authmode)
    {
        case WIFI_AUTH_OPEN:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_OPEN");
            break;
        case WIFI_AUTH_WEP:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_WEP");
            break;
        case WIFI_AUTH_WPA_PSK:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_WPA_PSK");
            break;
        case WIFI_AUTH_WPA2_PSK:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_WPA2_PSK");
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_WPA_WPA2_PSK");
            break;
        case WIFI_AUTH_ENTERPRISE:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_ENTERPRISE");
            break;
        case WIFI_AUTH_WPA3_PSK:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_WPA3_PSK");
            break;
        case WIFI_AUTH_WPA2_WPA3_PSK:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_WPA2_WPA3_PSK");
            break;
        case WIFI_AUTH_WAPI_PSK:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_WAPI_PSK");
            break;
        case WIFI_AUTH_OWE:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_OWE");
            break;
        case WIFI_AUTH_WPA3_ENT_192:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_WPA3_ENT_192");
            break;
        case WIFI_AUTH_WPA3_EXT_PSK:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_WPA3_EXT_PSK");
            break;
        case WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE");
            break;
        
        default:
            ESP_LOGI(TAG, "Authmode \t\tWIFI_AUTH_UNKNOWN");
            break;
    }
}

/**
 * @brief       打印WIFI加密类型
 * @param       pairwise_cipher :单播密码类型
 * @param       group_cipher    :群播密码类型
 * @retval      无
 */
static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_AES_CMAC128:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_AES_CMAC128");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    case WIFI_CIPHER_TYPE_AES_GMAC128:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_AES_GMAC128");
        break;
    case WIFI_CIPHER_TYPE_AES_GMAC256:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_AES_GMAC256");
        break;
    case WIFI_CIPHER_TYPE_UNKNOWN:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    default:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_AES_CMAC128:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_AES_CMAC128");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    case WIFI_CIPHER_TYPE_AES_GMAC128:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_AES_GMAC128");
        break;
    case WIFI_CIPHER_TYPE_AES_GMAC256:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_AES_GMAC256");
        break;
    case WIFI_CIPHER_TYPE_UNKNOWN:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    default:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}

void wifi_scan(void)
{
    char lcd_buf[100] = {0};
    /* 网卡初始化,即初始化 TCP/IP 协议栈 */
    ESP_ERROR_CHECK(esp_netif_init());
    /* 创建新的事件循环 */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /* 创建STA网络接口 */
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    /* WIFI初始化驱动 */
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;           /* 期望获取的AP数量，这里是12 */
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];   /* 存放扫描结果的数组 */
    uint16_t ap_count = 0;                              /* 实际扫描到的AP总数 */
    memset(ap_info, 0, sizeof(ap_info));

    /* 设置WIFI为STA模式 */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    /* 启动WIFI */
    ESP_ERROR_CHECK(esp_wifi_start());
    /* 开始扫描附近的WIFI */
    esp_wifi_scan_start(NULL, true);                    /* 使用默认扫描配置，且true阻塞扫描 */
    /* 获取上次上面的AP数量 */
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    /* 获取上次扫描找到的AP列表 */
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));    /*number既是输入参数也是输出参数*/

    ESP_LOGI(TAG, "Total APs scanned: %u", ap_count);
    /* 打印WIFI信息 */
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++)
    {
        sprintf(lcd_buf, "%s", ap_info[i].ssid);
        lcd_show_string(130, 20 * i, lcd_buf, RED ,WHITE, 16, 1);
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        print_auth_mode(ap_info[i].authmode);

        if (ap_info[i].authmode != WIFI_AUTH_WEP)                   /* WEP没有单播加密和组播加密概念 */
        {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }

        ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);      /* 打印主信道号 */
    }
    
}
