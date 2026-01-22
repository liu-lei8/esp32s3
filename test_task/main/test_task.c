#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

void taskA(void* param)
{
	while(1)
	{
		ESP_LOGI("main", "hello world");
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void app_main(void)
{
	xTaskCreatePinnedToCore(taskA, "helloworld", 2024, NULL, 3, NULL, 1);
}