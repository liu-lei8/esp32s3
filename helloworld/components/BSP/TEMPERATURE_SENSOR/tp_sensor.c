#include "tp_sensor.h"

esp_err_t ret_flag;
temperature_sensor_handle_t temp_handle = NULL;

void temperature_sensor_init(void)
{
    temperature_sensor_config_t temp_sensor_cfg = {
        .range_max = SENSOR_RANGE_MAX,
        .range_min = SENSOR_RANGE_MIN,
        .clk_src = TEMPERATURE_SENSOR_CLK_SRC_DEFAULT,
    };
    ret_flag |= temperature_sensor_install(&temp_sensor_cfg, &temp_handle);

    ESP_ERROR_CHECK(ret_flag);
}

float sensor_get_temperature(void)
{
    float temp;

    /*启用温度传感器*/
    ret_flag |= temperature_sensor_enable(temp_handle);

    /*获取传输的传感器数据*/
    ret_flag |= temperature_sensor_get_celsius(temp_handle, &temp);

    /*温度传感器使用完毕后，禁用温度传感器，节约功耗*/
    ret_flag |= temperature_sensor_disable(temp_handle);

    ESP_ERROR_CHECK(ret_flag);

    return temp;
}

