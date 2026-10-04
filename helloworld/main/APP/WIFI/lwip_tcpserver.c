#include "lwip_tcpserver.h"

/* 接收数据缓冲区 */
static uint8_t g_lwip_demo_recvbuf[LWIP_DEMO_RX_BUFSIZE];
/* 发送的数据内容 */
static char g_lwip_demo_sendbuf[] = " hello wifi \r\n";
/* 数据发送的标志位 */
uint8_t g_lwip_send_flag2;
static int g_sock_conn;                    /* 请求的socket */
static int g_lwip_connect_state = 0;
static const char* TAG = "lwip_tcpserver";

 /**
  * @brief       发送数据线程函数
  * @param       arg : 传入参数(未用到)
  * @retval      无
  */
static void lwip_send_thread(void* arg)
{
    arg = arg;

    while (1)
    {
        if ((g_lwip_send_flag2 & LWIP_SEND_DATA) == LWIP_SEND_DATA && g_lwip_connect_state == 1)
        {
            send(g_sock_conn, g_lwip_demo_sendbuf, sizeof(g_lwip_demo_sendbuf), 0);
            g_lwip_send_flag2 &= ~LWIP_SEND_DATA;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


void lwip_tcpserver_demo(void)
{
    struct sockaddr_in server_addr;     /* 服务器地址 */
    struct sockaddr_in conn_addr;       /* 连接的客户端地址 */
    socklen_t addr_len;                 /* 地址长度 */
    int sock_fd;
    char* tbuf;
    err_t err;
    int lenght;

    xTaskCreatePinnedToCore(lwip_send_thread, "lwip_send_thread", 4096, NULL, LWIP_SEND_THREAD_PRIO, NULL, 0);

    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);    /* 创建一个新的socket连接 */
    memset(&server_addr, 0, sizeof(server_addr));           /* 清空服务器地址 */
    server_addr.sin_family = AF_INET;                       /* 地址族，ipv4协议 */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);        /* 注意转化为网络字节序 */
    server_addr.sin_port = htons(LWIP_DEMO_PORT);           /* 指定端口号 */

    tbuf = malloc(200);
    sprintf(tbuf, "Port:%d", LWIP_DEMO_PORT);
    lcd_show_string(30, 190, tbuf, BLUE, WHITE, 16, 1);
    free(tbuf);

    err = bind(sock_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr)); /* 建立绑定 */

    if (err < 0)
    {
        ESP_LOGE(TAG, "bind failed: errno = %d(%s)", errno, strerror(errno));
        close(sock_fd);
        return;
    }

    err = listen(sock_fd, 4);   /* 监听连接请求，第二个参数表示已经完成连接、但还没被 accept() 取走的等待队列长度，即客户端数量 */
    if (err < 0)                /* 监听失败，则关闭套接字 */
    {
        perror("listen");
        close(sock_fd);
        return;
    }

    while (1)
    {
        g_lwip_connect_state = 0;
        addr_len = sizeof(struct sockaddr_in);

        g_sock_conn = accept(sock_fd, (struct sockaddr*)&conn_addr, &addr_len); /* 对监听到的请求进行连接, 默认是阻塞函数 */

        if (g_sock_conn < 0)    /* 状态小于0表示连接故障或者暂时没有客户端连接，那么就重新链接 */
        {
            ESP_LOGE(TAG, "accept failed: errno = %d(%s)", errno, strerror(errno));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        else 
        {
            lcd_fill(0, 210, 240, 280, WHITE);
            lcd_show_string(30, 210, "State: connection", BLUE, WHITE, 16, 1);
            g_lwip_connect_state = 1;
        }

        while (1)
        {
            memset(g_lwip_demo_recvbuf, 0, sizeof(g_lwip_demo_recvbuf));
            lenght = recv(g_sock_conn, g_lwip_demo_recvbuf, sizeof(g_lwip_demo_recvbuf), 0);    /* 接收数据 */

            if (lenght <= 0)
            {
                lcd_show_string(30, 210, "State: no connection", BLUE, WHITE, 16, 0);
                break;
            }

            printf("%s\r\n", g_lwip_demo_recvbuf);
        }

        if (g_sock_conn >= 0)
        {
            close(g_sock_conn);
        }
    }
}
