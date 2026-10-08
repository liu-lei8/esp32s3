#include "lwip_tcpclient.h"

/* 接收数据缓冲区 */
static uint8_t g_lwip_demo_recvbuf[LWIP_DEMO_RX_BUFSIZE];
/* 发送的数据内容 */
static char g_lwip_demo_sendbuf[] = " hello wifi \r\n";
/* 数据发送的标志位 */
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

    int sent_len;
    camera_fb_t* camera_frame = NULL;
    size_t total_sent = 0;
    size_t chunk_len;

    while (1)
    {
        /* 处于连接服务端状态 */
        if (g_lwip_connect_state == 1)
        {
            camera_frame = esp_camera_fb_get();         /* 获取OV5640摄像头像素数据 */
            if (camera_frame == NULL) {
                vTaskDelay(pdMS_TO_TICKS(1));
                continue;
            }

            total_sent = 0;
            while (total_sent < camera_frame->len)      /* 传输 */
            {
                // 一次性发送剩余所有数据，由 TCP 栈自己分片。这里传输帧不用加延迟，让send函数自然阻塞限速
                sent_len = send(g_sock, camera_frame->buf + total_sent, camera_frame->len - total_sent, 0);

                if (sent_len < 0)
                {
                    g_lwip_connect_state = 0;
                    ESP_LOGE(TAG, "send chunk fail, errno:%d", errno);
                    break;      /* 处理错误，例如断开连接或重试 */
                }
                else if (sent_len == 0)
                {
                     // 阻塞模式下，请求发送长度>0却返回0，说明连接可能已断开
                    ESP_LOGE(TAG, "send returned 0, connection may be closed");
                    g_lwip_connect_state = 0;
                    break;
                }
                else
                {
                    total_sent += sent_len;      /* 关键：检查实际发送的字节数，并累加 */
                }
            }
            esp_camera_fb_return(camera_frame);
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(1000));     /* 未连接时，延时等待，避免空转 */
        }

    }

}

void lwip_tcpclient_demo(void)
{
    struct sockaddr_in my_server_addr;
    err_t err;
    int recv_data_len;
    char host_ip[] = IP_ADDR;

    xTaskCreatePinnedToCore(lwip_send_thread, "lwip_send_thread", 4096, NULL, LWIP_SEND_THREAD_PRIO, NULL, 1);

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

        // int sndbuf = 65536;
        // setsockopt(g_sock, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));     /* 设置TCP发送缓冲区大小为65536字节,在menuconfig里直接设置lwip->tcp的传输大小 */

        int nodelay = 1;
        setsockopt(g_sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));    /* 避免小数据包被延迟合并发送，从而降低延迟 */

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

        vTaskDelay(pdMS_TO_TICKS(100));   // 给发送线程一点时间退出

        if (g_sock != -1)
        {
            shutdown(g_sock, 2);
            close(g_sock);
        }
    }
}
