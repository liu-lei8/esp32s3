#include "rng.h"

uint32_t rng_get_random_num(void)
{
    uint32_t randomNum;
    randomNum = esp_random();

    return randomNum;
}

int rng_get_random_range(int min, int max)
{
    uint32_t randomNum;
    randomNum = esp_random();

    return randomNum % (max - min + 1) + min;
}