#include "bsp/atk_dnesp32s3m.h"
#include "bsp/display.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcdfont.h"

static const char *TAG = "BSP_LCD";
static bsp_lcd_handles_t s_disp;

#define LCD_CMD_BITS   8
#define LCD_PARAM_BITS 8

static const unsigned char* get_font_data(uint8_t sizey) {
    if (sizey == 12) return lcd_asc2_1206[0];
    else if (sizey == 16) return lcd_asc2_1608[0];
    else if (sizey == 24) return lcd_asc2_2412[0];
    else if (sizey == 32) return lcd_asc2_3216[0];
    return lcd_asc2_1608[0]; // 默认 16
}

esp_err_t bsp_display_brightness_init(void)
{
    return xl9555_init_bsp();
}

esp_err_t bsp_display_brightness_deinit(void) { return ESP_OK; }

esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    // 只有开/关，不支持 PWM 调光
    return xl9555_pin_write_bsp(ATK_SLCD_PWR_IO, brightness_percent > 0);
}

esp_err_t bsp_display_backlight_off(void) { return bsp_display_brightness_set(0); }
esp_err_t bsp_display_backlight_on(void)  { return bsp_display_brightness_set(100); }

esp_err_t bsp_display_new(const bsp_display_config_t *config,
                          esp_lcd_panel_handle_t *ret_panel,
                          esp_lcd_panel_io_handle_t *ret_io)
{
    bsp_lcd_handles_t h;
    esp_err_t ret = bsp_display_new_with_handles(config, &h);
    if (ret == ESP_OK) {
        *ret_panel = h.panel;
        *ret_io = h.io;
    }
    return ret;
}

esp_err_t bsp_display_new_with_handles(const bsp_display_config_t *config, bsp_lcd_handles_t *ret_handles)
{
    ESP_RETURN_ON_FALSE(config && ret_handles, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "i2c init failed");
    ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), TAG, "brightness init failed");

    // 手动复位 LCD（通过 XL9555）
    ESP_RETURN_ON_ERROR(xl9555_pin_write_bsp(ATK_SLCD_RST_IO, 0), TAG, "rst low failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_RETURN_ON_ERROR(xl9555_pin_write_bsp(ATK_SLCD_RST_IO, 1), TAG, "rst high failed");
    vTaskDelay(pdMS_TO_TICKS(150));

    // SPI 总线
    const spi_bus_config_t buscfg = {
        .sclk_io_num = BSP_LCD_CLK,
        .mosi_io_num = BSP_LCD_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = config->max_transfer_sz,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "spi init failed");

    // Panel IO
    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = BSP_LCD_DC,
        .cs_gpio_num = BSP_LCD_CS,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 3,
        .trans_queue_depth = 10,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_cfg, &ret_handles->io),
                        TAG, "new io failed");


    // ST7789 面板
    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = -1,     // 已通过 XL9555 处理
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(ret_handles->io, &panel_cfg, &ret_handles->panel),
                        TAG, "new panel failed");

    esp_lcd_panel_reset(ret_handles->panel);
    esp_lcd_panel_init(ret_handles->panel);
    esp_lcd_panel_invert_color(ret_handles->panel, true);
    esp_lcd_panel_swap_xy(ret_handles->panel, false);
    esp_lcd_panel_mirror(ret_handles->panel, false, false);
    // 240x280 面板，可见区域 240x240，y 偏移 20 居中
    esp_lcd_panel_set_gap(ret_handles->panel, 0, 20);
    esp_lcd_panel_disp_on_off(ret_handles->panel, true);

    s_disp = *ret_handles;
    ESP_LOGI(TAG, "LCD initialized 240x240");
    return ESP_OK;
}

void bsp_display_delete(void)
{
    if (s_disp.panel) { esp_lcd_panel_del(s_disp.panel); s_disp.panel = NULL; }
    if (s_disp.io)    { esp_lcd_panel_io_del(s_disp.io); s_disp.io = NULL; }
    spi_bus_free(BSP_LCD_SPI_NUM);
}

bool bsp_display_lock(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return true;   // 无 LVGL 时，永远返回成功
}

void bsp_display_unlock(void)
{
    // 无 LVGL 时，什么都不做
}

void bsp_display_show_char(uint16_t x, uint16_t y, uint8_t chr, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t sizex = sizey / 2;
    uint8_t size = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizex; /* 一个字符的字节数 */
    const uint8_t* p;
    static uint16_t lcd_buf[32 * 16];       /* 因为esp_lcd_panel_draw_bitmap这个函数是异步的，必须加static */
    uint8_t temp;
    uint8_t idex;
    uint8_t y1 = 0, x1 = 0;
    uint8_t row = 0;

    uint16_t fc_swap = (fc >> 8) | (fc << 8);
    uint16_t bc_swap = (bc >> 8) | (bc << 8);
    p = get_font_data(sizey);

    idex = chr - 32;
    for (uint8_t i = 0; i < size; i++)  /* 填充颜色到lcd缓冲区 */
    {
        temp = p[idex * size + i];      /* 指向字符数组对应编号的第i个字节 */
        for (uint8_t j = 0; j < 8; j++)
        {
            if (temp & 0x80)
            {
                lcd_buf[y1 * sizex + x1] = fc_swap;
            }
            else
            {
                lcd_buf[y1 * sizex + x1] = bc_swap;
            }

            temp <<= 1;
            row++;
            if (row == sizey)
            {
                row = 0;
                x1++;
                y1 = 0;
                break;
            }
            else 
            {
                y1++;
            }
        }
    }
    
    x1 = x; y1 = y;
    if (mode)                           /* 叠加模式 */
    {
        for (uint8_t i = 0; i < sizey; i++)
        {
            for (uint8_t j = 0; j < sizex; j++)
            {
                if (lcd_buf[i * sizex + j] == fc_swap)
                {
                    esp_lcd_panel_draw_bitmap(s_disp.panel, x1 + j, y1 + i, x1 + j + 1, y1 + i + 1, &fc_swap);
                }
            }
        }
    }
    else                                /* 非叠加模式 */
    {
        esp_lcd_panel_draw_bitmap(s_disp.panel, x, y, x + sizex, y + sizey, lcd_buf);
        // 强制等待一下 DMA 发完，防止返回太快引发冲突
        vTaskDelay(pdMS_TO_TICKS(2)); 
    }

}

void bsp_display_show_string(uint16_t x, uint16_t y, const char* string, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t chr;
    uint16_t chr_num = strlen(string);

    for (uint8_t i = 0; i < chr_num; i++)
    {
        chr = *(string + i);
        bsp_display_show_char(x + i * (sizey / 2), y, chr, fc, bc, sizey, mode);
    }
}

void frame_printf(uint16_t *frame, int width, int height, int x, int y, uint16_t color, uint8_t sizey, const char *format, ...)
{
    char buffer[128]; 
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args); 
    va_end(args);

    uint16_t color_swap = (color >> 8) | (color << 8);
    int cursor_x = x;
    const uint8_t *font = get_font_data(sizey);
    uint8_t sizex = sizey / 2;
    uint8_t size = ((sizey + 7) / 8) * sizex; 
    
    for (int i = 0; buffer[i] != '\0'; i++) {
        uint8_t chr = buffer[i];
        if (chr < 32 || chr > 126) continue; 
        
        uint8_t idex = chr - 32;
        uint32_t byte_index = 0;

        for (uint8_t col = 0; col < sizex; col++) {
            for (uint8_t row = 0; row < sizey; row++) {
                if (row % 8 == 0) {
                    // 计算字库字节索引 (修正了之前的计算错误)
                    byte_index = idex * size + col * ((sizey + 7) / 8) + (row / 8);
                }
                uint8_t temp = font[byte_index];
                
                if (temp & (0x80 >> (row % 8))) {
                    int px = cursor_x + col;
                    int py = y + row;
                    if (px >= 0 && px < width && py >= 0 && py < height) {
                        frame[py * width + px] = color_swap;
                    }
                }
            }
        }
        cursor_x += sizex;
        if (cursor_x + sizex >= width) break; 
    }
}
