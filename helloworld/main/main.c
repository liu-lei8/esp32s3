#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iic_master.h"
#include "xl9555.h"
#include "nvs_flash.h"
#include "led.h"

void show_message(void)
{
    printf("\n");
    printf("*************************************\n");
    printf("ESP32-S3\n");
    printf("EXIO TEST\n");
    printf("AUTHOR@LIU-LEI\n");
    printf("KEY0:Beep On, KEY1:Beep Off\n");
    printf("KEY2:Led On, KEY3:Led Off\n");
    printf("*************************************\n");
    printf("\n");
}

/*还是得用命令链的形式探测已装载到总线上的设备, 此函数无效没用*/
static esp_err_t i2c_probe(i2c_master_bus_handle_t bus_handle, uint8_t addr)
{
    esp_err_t ret = ESP_FAIL;
    i2c_master_dev_handle_t dev_handle = NULL;
    uint8_t r_data = NULL;

    // i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    // i2c_master_start(cmd);
    // i2c_master_write_byte(cmd, addr << 1, ACK_CHECK_EN);
    // i2c_master_stop(cmd);
    // ret = i2c_master_cmd_begin(self->port, cmd, pdMS_TO_TICKS(50));
    // i2c_cmd_link_delete(cmd);

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = IIC_FREQ,
    };
    i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    ret = i2c_master_transmit(dev_handle, &r_data, 1, pdMS_TO_TICKS(10)); /*这里无论写还是读都返回ESP_OK,所以还是得用命令链的形式探测已装载到总线上的设备*/
    i2c_master_bus_rm_device(dev_handle);

    return ret;
}

static void i2c_scan(i2c_port_t port)
{
    i2c_master_bus_handle_t bus_handle = NULL;

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = true,
        .glitch_ignore_cnt = 7,
        .i2c_port = port,
        .scl_io_num = IIC0_SCL_GPIO_PIN,
        .sda_io_num = IIC0_SDA_GPIO_PIN,
    };
    i2c_new_master_bus(&bus_cfg, &bus_handle);

    printf("I2C scanning...\n");
    for (uint8_t addr = 0x08; addr < 0x78; ++addr)
    {
        if (i2c_probe(bus_handle, addr) == ESP_OK)
        {
            printf("Found I2C device at %#X\n", addr);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    i2c_del_master_bus(bus_handle);
}

void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();
    //i2c_scan(I2C_NUM_0);
    iic_obj_t* iic0_master = iic_init_new(I2C_NUM_0, 1, (int)XL9555_ADDR);
    xl9555_init(iic0_master);
    show_message();


    while (1)
    {
        vTaskDelay(200);
    }

    iic_delete_all_device(iic0_master, 1);
    i2c_del_master_bus(iic0_master->bus_handle);
}