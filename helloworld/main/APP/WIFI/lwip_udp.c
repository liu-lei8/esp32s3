#include "lwip_udp.h"

/* 接收数据缓冲区 */
static uint8_t g_lwip_demo_recvbuf[LWIP_DEMO_RX_BUFSIZE];
/* 发送的数据内容 */
static char g_lwip_demo_sendbuf[] = " hello wifi \r\n";
/* 数据发送的标志位 */
uint8_t g_lwip_send_flag;
static struct sockaddr_in dest_addr;                /* 远端地址 */
struct sockaddr_in g_local_info;
static socklen_t g_sock_fd;                                /* 定义一个socket接口 */


/**
 * @brief       发送数据线程函数
 * @param       arg : 传入参数(未用到)
 * @retval      无
 */
static void lwip_send_thread(void* arg)
{

    arg = arg;

    while(1)
    {
        if ((g_lwip_send_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA)
        {
            printf("send\r\n");
            sendto( g_sock_fd,                              /* socket */
                    g_lwip_demo_sendbuf,                    /* 要发送的数据 */
                    sizeof(g_lwip_demo_sendbuf), 0,         /* 发送数据的大小 */
                    (struct sockaddr*)&dest_addr,            /* 接收端地址信息 */
                    sizeof(dest_addr));                     /* 接收端地址信息大小 */

            g_lwip_send_flag &= ~LWIP_SEND_DATA;            /* 清除标志 */
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void lwip_udp_demo(void)
{
    char* tbuf;
    xTaskCreatePinnedToCore(lwip_send_thread, "lwip_send_thread", 4096, NULL, LWIP_SEND_THREAD_PRIO, NULL, 0);
    /* 远端参数设置 */
    dest_addr.sin_addr.s_addr = inet_addr(IP_ADDR);     /* 目标地址 */
    dest_addr.sin_family = AF_INET;                     /* address family使用ipv4的网络协议 */
    dest_addr.sin_port = htons(LWIP_DEMO_PORT);         /* 目标端口，htons：host to network short(该函数把16位两字节转换为大端字节序) */

    g_local_info.sin_addr.s_addr = htons(INADDR_ANY);   /* 设置本地IP地址 */
    g_local_info.sin_family = AF_INET;
    g_local_info.sin_port = htons(LWIP_DEMO_PORT);

    g_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);         /* 建立一个新的socket连接,类型为UDP类型 */

    tbuf = malloc(200);
    sprintf(tbuf, "Port:%d", LWIP_DEMO_PORT);
    lcd_show_string(30, 190, tbuf, MAGENTA, WHITE, 16, 1);

    /* socket与本地服务器地址建立绑定 */
    bind(g_sock_fd, (const struct sockaddr*)&g_local_info, sizeof(g_local_info));

    while(1)
    {
        memset(g_lwip_demo_recvbuf, 0, sizeof(g_lwip_demo_recvbuf));
        recv(g_sock_fd, (void*)g_lwip_demo_recvbuf, sizeof(g_lwip_demo_recvbuf), 0);    /* flag为0默认阻塞状态，有数据来才立马返回，宏MSG_WAITALL是接收完毕才返回 */
        printf("%s\r\n", g_lwip_demo_recvbuf);
    }
}
