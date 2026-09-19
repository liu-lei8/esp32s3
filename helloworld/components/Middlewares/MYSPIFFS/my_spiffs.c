#include "my_spiffs.h"

static const char* TAG = "spiffs";

esp_err_t spiffs_init(char* partition_label, char* mount_point, size_t max_files)
{
    size_t total_types = 0;
    size_t used_types = 0;

    /*配置spiffs文件系统参数*/
    esp_vfs_spiffs_conf_t spiffs_cfg = {
        .base_path = mount_point,               /*磁盘路径，比如"/0:","/1:"*/
        .partition_label = partition_label,     /*分区表的分区名称*/
        .max_files = max_files,                 /*最大可同时打开的文件数*/
        .format_if_mount_failed = true          /*挂载失败则重新格式化文件系统*/
    };

    /*初始化和挂载spiffs分区*/
    esp_err_t ret = esp_vfs_spiffs_register(&spiffs_cfg);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }

        return ESP_FAIL;
    }

    ret = esp_spiffs_info(spiffs_cfg.partition_label, &total_types, &used_types);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d Bytes, used: %d Bytes", total_types, used_types);
    }

    return ret;
}

esp_err_t spiffs_deinit(char* partition_label)
{
    return esp_vfs_spiffs_unregister(partition_label);
}

void spiffs_test(void)
{
    ESP_LOGI("spiffs_test", "Opening file");

    /*创建一个名为/spiffs/hello.txt的只写文件*/
    FILE* file_obj = fopen("/spiffs/hello.txt", "w");
    if (file_obj == NULL)
    {
        ESP_LOGE("spiffs_test", "Failed to open file for writing");
    }

    fprintf(file_obj, WRITE_DATA);  /*写入字符*/
    fclose(file_obj);               /*关闭文件*/
    ESP_LOGI("spiffs_test", "File written");

    /*重命名之前检查目标文件是否存在*/
    struct stat st;
    if (stat("/spiffs/foo.txt", &st) == 0)  /*获取文件信息，获取成功返回0*/
    {
        /*从文件系统中删除一个名称
        如果名称是文件的最后一个链接，并且没有其他进程将文件打开，名称对应的文件会实际被删除*/
        unlink("/spiffs/foo.txt");
    }

    /*重命名创建的文件*/
    ESP_LOGI("spiffs_test", "Renaming file");
    if (rename("/spiffs/hello.txt", "/spiffs/foo.txt") != 0)
    {
        ESP_LOGE("spiffs_test", "Rename failed");
    }

    /*打开重命名的文件并读取*/
    ESP_LOGI("spiffs_test", "Reading file");
    file_obj = fopen("/spiffs/foo.txt", "r");
    if (file_obj == NULL)
    {
        ESP_LOGE("spiffs_test", "Failed to open file for reading");
    }

    char line[64];
    fgets(line, sizeof(line), file_obj);    /*从指定的流中读取数据*/
    fclose(file_obj);

    char* pos = strchr(line, '\n'); /*指针pos指向第一个找到的'\n'*/
    if (pos)
    {
        *pos = '\0';    /*将'\n'替换成'\0'*/
    }

    ESP_LOGI("spiffs_test", "Read from file: '%s'", line);

    lcd_show_string(110, 140, line, BLUE, WHITE, 16, 0);
}
