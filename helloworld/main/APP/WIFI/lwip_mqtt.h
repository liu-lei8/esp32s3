#pragma once /* lwip_mqtt.h */

#include "mqtt_client.h"
#include "esp_log.h"
#include <time.h>

/* 用户需要根据OneNET设备信息完善以下宏定义 */
#define ONENET_PRODUCT_ID       "hk4zp7Z4GB"        /* 产品ID */
#define ONENET_DEVICE_NAME      "ESP32S3"           /* 设备名称 */
#define ONENET_DEVICE_KEY       "bHJqRGRGVHVhbnk5bm8zZXB2c3F2RkRIYlhjVmYyRGY="  /* 设备秘钥 */
#define ONENET_AUTH_TOKEN       "version=2018-10-31&res=products%2Fhk4zp7Z4GB%2Fdevices%2FESP32S3&et=1917075677&method=md5&sign=tMTC3q%2F4M6%2BIfQh21FE9eg%3D%3D"   /* 由三元组信息以及到期日期的事假戳计算出的访问Token */

/* MQTT地址与端口 */
#define ONENET_HOST_NAME        "studio-mqtt.heclouds.com"      /* OneNET非加密域名*/
#define ONENET_HOST_PORT        1883                            /* OneNET非加密端口*/

/* 根据OneNET鉴权规则计算得出的数值 */
#define ONENET_CLIENT_ID    ONENET_DEVICE_NAME      /* 客户端ID:设备名称 */
#define ONENET_USER_NAME    ONENET_PRODUCT_ID       /* 用户名：产品ID */
#define ONENET_PASSWORD     ONENET_AUTH_TOKEN       /* 密码：Token */

#define ONENET_DEVICE_PUBLISH       "$sys/hk4zp7Z4GB/ESP32S3/thing/property/post"   /* 发布属性 */
#define ONENET_DEVICE_SUBSCRIBE     "$sys/hk4zp7Z4GB/ESP32S3/thing/property/set"    /* 订阅属性 */

/* OneNET云的MQTT协议数据交互实验 */
void lwip_mqtt_demo(void);