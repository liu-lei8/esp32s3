#include "gt9xxx.h"

i2c_obj_t gt9xxx_i2c_master;
/*注意：除了GT9271支持10点触摸，其他触摸芯片只支持5点触摸*/
uint8_t gt_tnum = 5;    /*默认支持5点触摸*/
/*注：根据豆包的说法，读出的产id是1158，那么我手里的就是1158芯片，跟商家说的不一样。而1158同样支持最高10点触摸，目前实验最多5点触摸，10点触摸可能要设置寄存器*/

esp_err_t gt9xxx_write_reg(uint16_t reg, uint8_t* data, uint8_t len)
{
    uint8_t temp[2];

    temp[0] = reg >> 8;     /*先发高字节，再发低字节*/
    temp[1] = reg & 0xFF;

    i2c_buf_t buf[2] = {
        {.buf = temp, .len = 2},
        {.buf = data, .len =len}
    };

    return iic_transfer(&gt9xxx_i2c_master, GT9XXX_ADDR, 2, buf, IIC_FLAG_STOP);
}

esp_err_t gt9xxx_read_reg(uint16_t reg, uint8_t* data, uint8_t len)
{
    uint8_t temp[2];

    temp[0] = reg >> 8;     /*先发高字节，再发低字节*/
    temp[1] = reg & 0xFF;

    i2c_buf_t buf[2] = {
        {.buf = temp, .len = 2},
        {.buf = data, .len =len}
    };

    return iic_transfer(&gt9xxx_i2c_master, GT9XXX_ADDR, 2, buf, IIC_FLAG_WRITE | IIC_FLAG_READ | IIC_FLAG_STOP);
}

uint8_t gt9xxx_init(i2c_obj_t self)
{
    uint8_t temp[5];
    esp_err_t ret;

    if (self.init_flag)     /*初始化失败*/
    {
        gt9xxx_i2c_master = iic_init(I2C_NUM_0);
    }
    else
    {
        gt9xxx_i2c_master = self;
    }

    gpio_config_t gt9xxx_int_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ull << GT9XXX_INT_GPIO_PIN,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&gt9xxx_int_cfg);

    for (uint8_t i = 0; i < 2; i++)     /*硬复位两次*/
    {
        CT_RST(0);
        vTaskDelay(200);
        CT_RST(1);
        vTaskDelay(200);
    }

    ret = gt9xxx_read_reg(GT9XXX_PID_REG, temp, 4);
    if (ret != ESP_OK)
    {
        printf("gt9xxx_read_reg GT9XXX_PID_REG fail!\n");
        return 1;       /*读取失败，则初始化失败*/
    }

    temp[4] = 0;
    printf("CTP GT9XXX ID: %s\n", temp);

    /*判断是否是特定的触摸屏*/
    if (strcmp((char*)temp, "1158") && strcmp((char*)temp, "9147") && strcmp((char*)temp, "911") && strcmp((char*)temp, "9271"))
    {
        return 1;   /*若不是用到以上特定触摸屏，初始化失败，则需要查看硬件触摸IC型号和初始化序列*/
    }

    if (strcmp((char*)temp, "9271") == 0)   /*ID：9271，支持10点触摸*/
    {
        gt_tnum = 10;
    }

    temp[0] = 0x02;
    gt9xxx_write_reg(GT9XXX_CTRL_REG, temp, 1);     /*软复位*/
    vTaskDelay(10);

    temp[0] = 0x00;
    gt9xxx_write_reg(GT9XXX_CTRL_REG, temp, 1);     /*结束复位进入读取坐标状态*/

    return 0;
}

/*GTGXXX的十个触摸点（最多），对应的触摸寄存器表*/
const uint16_t GT9XXX_TPX_TBL[10] = {
    GT9XXX_TP1_REG, GT9XXX_TP2_REG, GT9XXX_TP3_REG, GT9XXX_TP4_REG, GT9XXX_TP5_REG,
    GT9XXX_TP6_REG, GT9XXX_TP7_REG, GT9XXX_TP8_REG, GT9XXX_TP9_REG, GT9XXX_TP10_REG
};

uint8_t gt9xxx_scan(uint8_t mode)
{
    static uint8_t t = 0;           /*控制查询间隔，用于节省CPU占用率*/
    uint8_t i = 0;
    uint16_t temp = 0;
    uint16_t tempsta = 0;
    uint8_t buf[4];
    uint8_t ret = 0;    /*返回值，默认无触点*/

    t++;
    if ((t % 10 == 0) || t < 10) /*空闲时，每十次查询一次，检测到触点时连续查询10次提高准确率*/
    {
        gt9xxx_read_reg(GT9XXX_GSTID_REG, &mode, 1);    /*读取触摸点的状态以及数量*/
        
        if ((mode & 0x80) && ((mode & 0xF) <= gt_tnum))  /*有触点，且小于等于最大触点数量*/
        {
            i = 0;
            gt9xxx_write_reg(GT9XXX_GSTID_REG, &i, 1);  /*清空触摸状态，以便读取下次触摸状态*/
        }

        if ((mode & 0xF) && ((mode & 0xF) <= gt_tnum))
        {
            ret = 1;    /*返回值有触点*/
            temp = 0xFFFF << (mode & 0xF);  /*将触摸点数量转换为1的位数,匹配tp_dev.sta的定义*/
            tempsta = tp_dev.sta;   /*保存上一次的触摸状态*/
            tp_dev.sta = ~temp | TP_PRES_DOWN | TP_CATH_PRES;
            tp_dev.x[gt_tnum - 1] = tp_dev.x[0];    /*保存触摸点0的数据到最后一个触摸点上*/
            tp_dev.y[gt_tnum - 1] = tp_dev.y[0];

            for (i = 0; i < gt_tnum; i++)
            {
                if (tp_dev.sta & (1 << i))      /*判断触摸点是否有效*/
                {
                    gt9xxx_read_reg(GT9XXX_TPX_TBL[i], buf, 4); /*读取xy坐标值*/
                    if (tp_dev.touchtype & 0x01)    /*横屏*/
                    {
                        tp_dev.x[i] = ltdcdev.width - (((uint16_t)buf[1] << 8) | buf[0]);
                        tp_dev.y[i] = ltdcdev.height - (((uint16_t)buf[3] << 8) | buf[2]);
                    }
                    else    /*竖屏*/
                    {
                        tp_dev.x[i] = ltdcdev.width - (((uint16_t)buf[3] << 8) | buf[2]);
                        tp_dev.y[i] = ((uint16_t)buf[1] << 8) | buf[0];
                    }
                }
            }

            /*第一个坐标为非法数据*/
            if (tp_dev.x[0] > ltdcdev.width || tp_dev.y[0] > ltdcdev.height)
            {
                if ((mode & 0xF) > 1)   /*第一个触点无效但是有其他触点，将第二触点复制给第一*/
                {
                    tp_dev.x[0] = tp_dev.x[1];
                    tp_dev.y[0] = tp_dev.y[1];
                    t = 0;  /*有合法触点，至少会连续监测十次，从而提高命中率*/
                }
                else    /*否则将第一个触点和状态还原为上一次的*/
                {
                    tp_dev.x[0] = tp_dev.x[gt_tnum - 1];
                    tp_dev.y[0] = tp_dev.y[gt_tnum - 1];
                    tp_dev.sta = tempsta;
                    mode = 0x80;    /*有触点但是为无效触点*/
                }
            }
            else    /*不是非法数据*/
            {
                t = 0;  /*有合法触点，至少会连续监测十次，从而提高命中率*/
            }
        }

    }

    /*只有一个触点且无效的状态下，重置状态或坐标*/
    if ((mode & 0x8F) == 0x80)
    {
        if (tp_dev.sta & TP_PRES_DOWN)      /*上次为按下状态*/
        {
            tp_dev.sta &= ~TP_PRES_DOWN;    /*标记按键松开*/
        }
        else
        {
            tp_dev.x[0] = 0xFFFF;
            tp_dev.y[0] = 0xFFFF;
            tp_dev.sta &= 0xC000;       /*清除触点位标记，保留最高两位触点状态*/
        }
    }

    if (t > 240)
    {
        t = 10;     /*重新从10开始计数*/
    }

    return ret;
}
