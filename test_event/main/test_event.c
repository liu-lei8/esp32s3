// #include "freertos/FreeRTOS.h"
// #include "freertos/event_groups.h"
// #include "freertos/task.h"
// #include "esp_log.h"

// #define NUM0_BIT	BIT0
// #define NUM1_BIT	BIT1

// EventGroupHandle_t event_handle;

// void taskA(void* param)
// {
// 	while(1)
// 	{
// 		xEventGroupSetBits(event_handle, NUM0_BIT);
// 		vTaskDelay(pdMS_TO_TICKS(1000));
// 		xEventGroupSetBits(event_handle, NUM1_BIT);
// 		vTaskDelay(pdMS_TO_TICKS(1000));
// 	}
// }

// void taskB(void* param)
// {
// 	EventBits_t ev;
// 	while(1)
// 	{
// 		ev = xEventGroupWaitBits(event_handle, NUM0_BIT | NUM1_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
// 		if (ev & NUM0_BIT)
// 		{
// 			ESP_LOGI("ev", "Get bit0 event");
// 		}
// 		if (ev & NUM1_BIT)
// 		{
// 			ESP_LOGI("ev", "Get bit1 event");
// 		}
// 	}
// }

// void app_main(void)
// {
// 	event_handle = xEventGroupCreate();
// 	xTaskCreatePinnedToCore(taskA, "set bit", 2048, NULL, 3, NULL, 1);
// 	xTaskCreatePinnedToCore(taskB, "wait bit", 2048, NULL, 3, NULL, 1);
// }
