#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

QueueHandle_t queue_handle = NULL;      //任务句柄

typedef struct 
{
    int value;
}queue_data_t;

void taskA(void* param)
{
    //从队列接受数据并打印
    queue_data_t data;
   
    while (1)
    {
        if (xQueueReceive(queue_handle, &data, 100) == pdTRUE)
        {
            ESP_LOGI("queue", "receive queue value: %d", data.value);
        }
    }

}

void taskB(void* param)
{
    //每隔一秒向队列发送数据
    queue_data_t data;
    memset(&data, 0, sizeof(queue_data_t));
    while (1)
    {
        xQueueSend(queue_handle, &data, 100);
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        data.value++;
    }
}

void app_main(void)
{
    queue_handle = xQueueCreate(10, sizeof(queue_data_t));
    xTaskCreatePinnedToCore(taskA, "taskA", 2048, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(taskB, "taskB", 2048, NULL, 3, NULL, 1);
}
