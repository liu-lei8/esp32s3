// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "nvs_flash.h"
// #include "esp_system.h"
// #include "esp_chip_info.h"
// #include "esp_flash.h"
// #include "esp_psram.h"

// void app_main(void)
// {
//     esp_err_t ret;
//     uint32_t flash_size;
//     esp_chip_info_t chip_info;      //定义芯片信息结构体变量

//     ret = nvs_flash_init();         //初始化nvs
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
//     {
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         ret = nvs_flash_init();
//     }

//     esp_chip_info(&chip_info);                  //得到芯片信息
//     esp_flash_get_size(NULL, &flash_size);      //得到主SPI的FLASH大小
//     //打印芯片内核数量信息
//     printf("内核：CPU数量%hu\n", chip_info.cores);
//     //打印FLASH大小
//     printf("FLASH SIZE: %lu MB flash\n", flash_size / (1024 * 1024));
//     //打印PSRAM大小
//     printf("PSRAM SIZE: %d bytes\n", esp_psram_get_size());

//     while (1)
//     {
//         printf("Hello-esp32\r\n");
//         vTaskDelay(1000);
//     }
// }