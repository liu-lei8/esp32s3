#pragma once    /*my_spiffs.h*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spiffs.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "esp_log.h"
#include "lcd.h"

#define DEFAULT_FD_NUM      5                       /*默认最大可同时打开文件数量*/
#define DEFAULT_MOUNT_POINT "/spiffs"               /*文件系统名称*/
#define WRITE_DATA          "Hello ESP32-S3\n"


/**
 * @brief spiffs文件系统初始化
 * @param partition_label:分区表的分区名称
 * @param mount_point:文件系统关联的文件路径前缀
 * @param max_files:可以同时打开的最大文件数
 * @return ESP_OK:成功；ESP_FAIL:失败
 */
esp_err_t spiffs_init(char* partition_label, char* mount_point, size_t max_files);

esp_err_t spiffs_deinit(char* partition_label);

void spiffs_test(void);