#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dht11.h"
#include "driver/gpio.h"
#include "inttypes.h"

SemaphoreHandle_t mutex_sem;

void taskA(void* param)
{
	int temp;
	int humidity;
	while(1)
	{
		
		if(xSemaphoreTake(mutex_sem, portMAX_DELAY) == pdTRUE)
		{
			DHT11_StartGet(&temp, &humidity);
			ESP_LOGI("dht11", "taskA:temp->%d,humidity->%d%%", temp / 10, humidity);
		}
		xSemaphoreGive(mutex_sem);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void taskB(void* param)
{
	int temp;
	int humidity;
	while(1)
	{
		if(xSemaphoreTake(mutex_sem, portMAX_DELAY) == pdTRUE)
		{
			DHT11_StartGet(&temp, &humidity);
			ESP_LOGI("dht11", "taskB:temp->%d,humidity->%d%%", temp / 10, humidity);
		}
		xSemaphoreGive(mutex_sem);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void app_main(void)
{
	DHT11_Init(GPIO_NUM_6);
	mutex_sem = xSemaphoreCreateMutex();
	xTaskCreatePinnedToCore(taskA, "semaGive", 2048, NULL, 3, NULL,1);
	xTaskCreatePinnedToCore(taskB, "semaTake", 2048, NULL, 3, NULL,1);
}