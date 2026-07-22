#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "spi_sd.h"
#include "fonts.h"
#include "text.h"
#include "piclib.h"

i2c_obj_t i2c0_master;

void app_main(void)
{
    esp_err_t ret;
    uint8_t key = 0;
    FF_DIR picdir;
    uint16_t totpicnum;
    FILINFO* picfileinfo;
    char* pname;
    uint32_t* picoffsettbl;
    uint16_t curindex = 0;
    uint16_t temp;
    uint8_t t = 0;
    uint8_t pause = 0;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    led_init();
    i2c0_master = iic_init(I2C_NUM_0);
    spi2_init();
    xl9555_init(i2c0_master);
    lcd_init();

    if (sd_spi_init())
    {
        lcd_show_string(30, 110, "SD Card Error!", RED, WHITE, 16, 1);
        return;
    }

    ret = exfuns_init();

    while (fonts_init())
    {
        lcd_clear(WHITE);
        lcd_show_string(30, 30, "ESP32-S3", RED, WHITE, 16, 1);

        key = fonts_update_font(30, 50, 16, (uint8_t*)"0:", RED, WHITE);

        if (key)
        {
            lcd_show_string(30, 50, "Font Update Failed!", RED, WHITE, 16, 1);
            return;
        }

        lcd_show_string(30, 50, "Font Update Success!", RED, WHITE, 16, 1);
        vTaskDelay(1000);
        lcd_clear(WHITE);
    }

    lcd_clear(WHITE);
    text_show_string(30, 50, 200, 16, "正点原子ESP32-S3开发板", 16, 1, RED, WHITE);
    text_show_string(30, 70, 200, 16, "图片显示实验", 16, 1, RED, WHITE);
    text_show_string(30, 90, 200, 16, "作者@LIU-LEI", 16, 1, RED, WHITE);
    text_show_string(30, 110, 200, 16, "KEY0:NEXT KEY1:PREV", 16, 1, RED, WHITE);
    text_show_string(30, 130, 200, 16, "KEY3:PAUSE", 16, 1, RED, WHITE);

    if (f_opendir(&picdir, "0:/PICTURE0"))
    {
        text_show_string(30, 150, 200, 16, "PICTURE文件夹打开错误", 16, 1, RED, WHITE);
        return;
    }

    totpicnum = pic_get_tnum("0:/PICTURE0");
    if (totpicnum == 0)
    {
        text_show_string(30, 150, 200, 16, "没有图片文件", 16, 1, RED, WHITE);
        return;
    }

    picfileinfo = (FILINFO*)malloc(sizeof(FILINFO));
    pname = malloc(255 * 2 + 1);
    picoffsettbl = (uint32_t*)malloc(4 * totpicnum);

    if (!picfileinfo || !pname || !picoffsettbl)
    {
        text_show_string(30, 150, 200, 16, "内存分配失败", 16, 1, RED, WHITE);
        return;
    }

    ret = f_opendir(&picdir, "0:/PICTURE0");

    if (ret == FR_OK)
    {
        curindex = 0;

        while (1)
        {
            temp = picdir.dptr;         /*获取目录中目录项的字节偏移量*/
            ret = f_readdir(&picdir, picfileinfo);
            if (ret != FR_OK || picfileinfo->fname[0] == 0) break;

            ret = exfuns_file_type(picfileinfo->fname);
            if ((ret & 0x0F) != 0x00)/*这个PICTURE文件夹里只有图片和database文件才能这么判断*/
            {
                picoffsettbl[curindex] = temp;  /*将各个目录项偏移量放入表中*/
                curindex++;
            }
        }

        ESP_LOGI("main", "0:/PICTURE pic_num:%d", curindex);
    }

    text_show_string(30, 150, 200, 16, "图片开始显示……", 16, 1, RED, WHITE);
    vTaskDelay(1000);
    piclib_init();
    curindex = 0;

    ret = f_opendir(&picdir, (const TCHAR*)"0:/PICTURE0");
    while (ret == FR_OK)
    {
        atk_dir_sdi(&picdir, picoffsettbl[curindex]);   /*进行偏移找到对应的图片*/
        ret = f_readdir(&picdir, picfileinfo);          /*读取目录下的一个文件*/

        if (ret != FR_OK || picfileinfo->fname[0] == 0) break;  /*错误或到末尾了，退出*/

        strcpy(pname, "0:/PICTURE0/");
        strcat(pname, picfileinfo->fname);
        lcd_clear(BLACK);

        piclib_ai_load_picfile(pname, 0, 0, lcd_self.width,  lcd_self.height);/*显示图片*/
        text_show_string(20, 2, lcd_self.width, 16, pname, 16, 1, RED, WHITE);/*显示图片名*/

        t = 1;                                  /*这里为1，暂停图片时，LED才不会一直闪*/
        while (1)
        {
            key = xl9555_key_scan(0);           /*扫描按键*/

            if (t > 250) key = KEY0_PRES;     /*模拟一次按下KEY0*/

            if (t % 20 == 0)    LED_TOGGLE();

            if (key == KEY1_PRES)               /*上一张图片*/
            {
                if (curindex) curindex--;
                else curindex = totpicnum - 1;
                break;
            }
            else if (key == KEY0_PRES)          /*下一张图片*/
            {
                curindex++;
                if (curindex >= totpicnum) curindex = 0;    /*最后一张了从头开始*/
                break;
            }
            else if (key == KEY3_PRES)          /*暂停或开启图片循环*/
            {
                pause = !pause;
                LED(!pause);                     /*暂停时LED常亮*/
            }

            if (pause == 0) t++;
            vTaskDelay(100);
        }
        ret = 0;
    }

    free(picfileinfo);
    free(pname);
    free(picoffsettbl);
}