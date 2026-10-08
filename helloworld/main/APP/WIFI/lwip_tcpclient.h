#pragma once /* lwip_tcpclient.h */

#include <sys/socket.h>
#include "lwip/err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "camera.h"

#define LWIP_SEND_DATA  0X80
#define IP_ADDR                 "192.168.247.99"
#define LWIP_DEMO_RX_BUFSIZE    200                     /* 最大接收数据长度 */
#define LWIP_DEMO_PORT          8080                    /* 连接远端服务端端口号 */
#define LWIP_SEND_THREAD_PRIO   (tskIDLE_PRIORITY + 3)  /* 发送数据优先级 */


extern uint8_t g_lwip_send_flag1;

/* TCP Client实验 */
void lwip_tcpclient_demo(void);