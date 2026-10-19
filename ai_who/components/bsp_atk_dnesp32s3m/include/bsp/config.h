#pragma once

// 使用无图形库模式（WHO 直接调用 esp_lcd，不需要 LVGL）
// 如果你以后要用 LVGL，把这个宏改为 0
#define BSP_CONFIG_NO_GRAPHIC_LIB (1)