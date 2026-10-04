#pragma once        /* lwip_tcpserver.h */

#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd.h"

#define LWIP_SEND_DATA  0X80
#define LWIP_DEMO_RX_BUFSIZE    200                     /* 最大接收数据长度 */
#define LWIP_DEMO_PORT          8080                    /* 连接远端服务端端口号 */
#define LWIP_SEND_THREAD_PRIO   (tskIDLE_PRIORITY + 3)  /* 发送数据优先级 */

extern uint8_t g_lwip_send_flag2;

/* TCP server实验 */
void lwip_tcpserver_demo(void);