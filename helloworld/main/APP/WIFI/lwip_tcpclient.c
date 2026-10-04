#include "lwip_tcpclient.h"

/* 接收数据缓冲区 */
static uint8_t g_lwip_demo_recvbuf[LWIP_DEMO_RX_BUFSIZE];
/* 发送的数据内容 */
static char g_lwip_demo_sendbuf[] = " hello wifi \r\n";
/* 数据发送的标志位 */
uint8_t g_lwip_send_flag1;
static int g_sock = -1;                    /* 定义的socket接口，即套接字*/
static int g_lwip_connect_state = 0;       /* 连接服务端的标志 */
static const char* TAG = "lwip_tcpclient";

 /**
  * @brief       发送数据线程函数
  * @param       arg : 传入参数(未用到)
  * @retval      无
  */
static void lwip_send_thread(void* arg)
{
    arg = arg;

    err_t err;

    while (1)
    {
        while (1)
        {
            /* 有数据要发送且处于连接服务端状态 */
            if ((g_lwip_send_flag1 & LWIP_SEND_DATA) == LWIP_SEND_DATA && g_lwip_connect_state == 1)
            {
                printf("send\r\n");
                err = send(g_sock, g_lwip_demo_sendbuf, sizeof(g_lwip_demo_sendbuf), 0);

                if (err < 0)
                {
                    break;
                }

                g_lwip_send_flag1 &= ~LWIP_SEND_DATA;        /* 清楚标志 */
            }

            vTaskDelay(10);
        }

        shutdown(g_sock, 1);    /* 第二个参数填0表示禁止接收数据，1表示禁止发送数据，2读写都禁止 */
        close(g_sock);    /* 释放资源 */
    }
}

void lwip_tcpclient_demo(void)
{
    struct sockaddr_in my_server_addr;
    err_t err;
    int recv_data_len;
    char host_ip[] = IP_ADDR;

    xTaskCreatePinnedToCore(lwip_send_thread, "lwip_send_thread", 4096, NULL, LWIP_SEND_THREAD_PRIO, NULL, 0);

    while (1)
    {
sock_start:
        g_lwip_connect_state = 0;
        inet_pton(AF_INET, host_ip, &my_server_addr.sin_addr);  /* presentation to network把十进制的文本转换为网络协议能识别的二进制格式*/
        my_server_addr.sin_family = AF_INET;
        my_server_addr.sin_port = htons(LWIP_DEMO_PORT);
        g_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);      /* 第三个参数填0表示根据第二个参数自动选择对应协议，这里是TCP协议*/
        memset(&my_server_addr.sin_zero, 0, sizeof(my_server_addr.sin_zero));

        /* 连接远程IP地址 */
        err = connect(g_sock, (const struct sockaddr*)&my_server_addr, sizeof(my_server_addr));

        if (err == -1)
        {
            ESP_LOGE(TAG, "Socket unable to connect:err %d", err);
            shutdown(g_sock, 2);
            close(g_sock);
            vTaskDelay(pdMS_TO_TICKS(1000));
            goto sock_start;
        }

        g_lwip_connect_state = 1;
        ESP_LOGI(TAG, "Successfully connected");

        while (1)
        {
            recv_data_len = recv(g_sock, g_lwip_demo_recvbuf, sizeof(g_lwip_demo_recvbuf) - 1, 0);

            if (recv_data_len < 0)
            {
                g_lwip_connect_state = 0;
                ESP_LOGE(TAG, "recv failed: err %d", err);
                break;
            }
            else if (recv_data_len == 0)        /* 表示服务器关闭了连接 */
            {
                g_lwip_connect_state = 0;
                ESP_LOGE(TAG, "server closed the connection: err %d", err);
                break;
            }
            else 
            {
                g_lwip_demo_recvbuf[recv_data_len] = 0;
                ESP_LOGI(TAG, "Received %d types from %s", recv_data_len, host_ip);
                ESP_LOGI(TAG, "%s", g_lwip_demo_recvbuf);
            }
        }

        if (g_sock != -1)
        {
            shutdown(g_sock, 2);
            close(g_sock);
        }
    }
}
