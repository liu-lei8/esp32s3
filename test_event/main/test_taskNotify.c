#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

TaskHandle_t taskA_handle;
TaskHandle_t taskB_handle;

void taskA(void* param)
{
	uint32_t value = 0;
	vTaskDelay(pdMS_TO_TICKS(200));
	while(1)
	{
		xTaskNotify(taskB_handle, value, eSetValueWithOverwrite);
		vTaskDelay(pdMS_TO_TICKS(1000));
		value++;
	}
}

void taskB(void* param)
{
	uint32_t value;
	while(1)
	{
		xTaskNotifyWait(0x00, ULONG_MAX, &value, portMAX_DELAY);
		ESP_LOGI("notify", "notify wait value:%lu", value);
	}
}

void app_main(void)
{
	xTaskCreatePinnedToCore(taskA, "send notify", 2048, NULL, 3, &taskA_handle, 1);
	xTaskCreatePinnedToCore(taskB, "wait notify", 2048, NULL, 3, &taskB_handle, 1);
}