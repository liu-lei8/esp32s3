#include "driver/gptimer.h"
#include "led.h"
#include "esp_clk_tree.h"

typedef struct {
    gptimer_handle_t gptimer_handle;
    gptimer_clock_source_t clk_src;
    uint64_t timing_time;
    uint64_t alarm_value;
    uint64_t gptimer_count_value;
    bool auto_reload;
}user_gptimer_config_t;

void user_gptimer_init(user_gptimer_config_t* user_gptimer_cfg);