#include "adc.h"

adc_oneshot_unit_handle_t adc_handle = NULL;    /*adc句柄*/

void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT_1,           /*ADC单元：ADC1/ADC2*/
        .ulp_mode = ADC_ULP_MODE_DISABLE,/*Ultra Low Power不使用超低功耗模式，在主MUC上工作*/
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc_handle));/*ADC初始化（单次转换模式）*/

    adc_oneshot_chan_cfg_t adc_chan = {
        .atten = ADC_ATTEN_DB_12,       /*ADC衰减*/
        .bitwidth = ADC_BITWIDTH_12,    /*ADC分辨率*/
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHAN, &adc_chan));
}

uint16_t adc_get_result_average(adc_channel_t ch, uint32_t times)
{
    uint32_t sum = 0;
    uint16_t temp_val = 0;

    int* rawdata = heap_caps_malloc(times * sizeof(int), MALLOC_CAP_INTERNAL);
    if (rawdata == NULL)
    {
        ESP_LOGE("adc", "Memory for adc is not enough");
        return EXIT_FAILURE;
    }

    for (uint32_t t = 0; t < times; t++)
    {
        adc_oneshot_read(adc_handle, ch, &rawdata[t]);
        vTaskDelay(5);      /*延迟给ADC内部电容足够的充电时间,以平滑毛刺*/
    }

    for (uint16_t i = 0; i < times - 1; i++)
    {
        for (uint16_t j = i + 1; j < times; j++)
        {
            if (rawdata[i] > rawdata[j])       /*升序排序*/
            {
                temp_val = rawdata[i];
                rawdata[i] = rawdata[j];
                rawdata[j] = temp_val;
            }
        }
    }

    for (uint32_t i = LOST_VAL; i < times - LOST_VAL; i++)
    {
        sum += rawdata[i];      /*去掉首尾两端极值*/
    }
    heap_caps_free(rawdata);

    return sum / (times - 2 * LOST_VAL);
}
