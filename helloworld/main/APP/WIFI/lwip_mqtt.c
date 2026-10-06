#include "lwip_mqtt.h"

int g_publish_flag = 0;
static const char* TAG = "MQTT_ONENET";

/**
 * @brief       错误日记
 * @param       message     :错误消息
 * @param       error_code  :错误码
 * @retval      无
 */
static void log_error_if_nonzero(const char* message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: %#x",message, error_code);
    }
}

/**
 * @brief       WIFI链接回调函数
 * @param       arg:传入网卡控制块
 * @param       event_base:MQTT事件
 * @param       event_id:事件ID
 * @param       event_data:事件数据
 * @retval      无
 */
static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base = %s, event_id = %" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:          /* 连接事件 */
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            /* 订阅OneNET平台下发的命令主题 */
            msg_id = esp_mqtt_client_subscribe(client, ONENET_DEVICE_SUBSCRIBE, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            g_publish_flag = 1;
            break;
        case MQTT_EVENT_DISCONNECTED:       /* 断开连接事件 */
            ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:         /* 订阅成功事件 */
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:       /* 取消订阅事件 */
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:          /* 发布成功事件 */
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:               /* 接收数据事件 */
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
            {
                log_error_if_nonzero("reported form esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void lwip_mqtt_demo(void)
{
    char mqtt_publish_data[256];
    float temp_value = 33.3;

    /* 动态构造JSON格式数据 */
    snprintf(mqtt_publish_data, sizeof(mqtt_publish_data),
            "{"
                "\"id\":\"123\","
                "\"version\":\"1.0\","
                "\"params\":{"
                    "\"CurrentTemperature\":{\"value\":%.2f}"
                "}"
            "}",
            temp_value);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.hostname = ONENET_HOST_NAME,                /* OneNET MQTT地址 */
        .broker.address.port = ONENET_HOST_PORT,                    /* OneNET端口号 */
        .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,        /* TCP模式 */
        .credentials.client_id = ONENET_CLIENT_ID,                  /* 设备名称 */
        .credentials.username = ONENET_USER_NAME,                   /* 产品ID */
        .credentials.authentication.password = ONENET_PASSWORD,     /* 计算出的Token */
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    while (1)
    {
        if (g_publish_flag == 1)
        {
            esp_mqtt_client_publish(client, ONENET_DEVICE_PUBLISH, mqtt_publish_data, sizeof(mqtt_publish_data), 1, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

