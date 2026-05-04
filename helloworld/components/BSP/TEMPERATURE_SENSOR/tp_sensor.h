#pragma once

#include "driver/temperature_sensor.h"

#define SENSOR_RANGE_MIN    20      /*要测量的温度最小值*/
#define SENSOR_RANGE_MAX    50      /*要测量的温度最大值*/

void temperature_sensor_init(void);
float sensor_get_temperature(void);