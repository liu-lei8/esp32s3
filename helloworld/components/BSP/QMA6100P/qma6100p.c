#include "qma6100p.h"

i2c_obj_t qma6100p_i2c_master;

static esp_err_t qma6100p_reg_write_byte(uint8_t reg, uint8_t data)
{
    i2c_buf_t bufs[2] = {
        {.buf = &reg, .len = 1},
        {.buf = &data, .len = 1},
    };
    return iic_transfer(&qma6100p_i2c_master, QMA6100P_ADDR, 2, bufs, IIC_FLAG_STOP);
}

static esp_err_t qma6100p_reg_read(const uint8_t reg, uint8_t* data, const size_t len)
{
    i2c_buf_t bufs[2] = {
        {.buf = &reg, .len = 1},
        {.buf = data, .len = len},
    };
    return iic_transfer(&qma6100p_i2c_master, QMA6100P_ADDR, 2, bufs, IIC_FLAG_WRITE | IIC_FLAG_READ | IIC_FLAG_STOP);
}

/**
 * @brief 配置qma6100p的步进中断的引脚映射和使能
 * @param int_map:映射到qma6100p的哪个中断引脚
 * @param enable ：1:使能，0：失能
 */
static void qma6100p_step_int_config(qma6100p_int_map int_map, uint8_t enable)
{
    /*1.读取当前中断使能寄存器状态*/
    uint8_t reg_0x16;
    qma6100p_reg_read(0x16, &reg_0x16, 1);

    /*根据enable参数设置步进中断使能位（0x16位3）*/
    if (enable)
    {
        reg_0x16 |= (1 << 3);   /*置位STEP_IEN,使能步进中断*/
    }
    else
    {
        reg_0x16 &= ~(1 << 3);  /*清零STEP_IEN,禁止步进中断*/
    }
    qma6100p_reg_write_byte(0x16, reg_0x16);

    /*根据int_map参数配置中断映射*/
    if (int_map == QMA6100P_MAP_INT1)
    {
        /*映射到INT1引脚，置位0x19<3>,清除0x1B<3>*/
        uint8_t reg_0x19 = 0;
        qma6100p_reg_read(0x19, &reg_0x19, 1);
        reg_0x19 |= (1 << 3);       /*INT1_STEP = 1*/
        qma6100p_reg_write_byte(0x19, reg_0x19);

        uint8_t reg_0x1B = 0;
        qma6100p_reg_read(0x1B, &reg_0x1B, 1);
        reg_0x1B &= ~(1 << 3);      /*INT2_STEP = 0*/
        qma6100p_reg_write_byte(0x1B, reg_0x1B);
    }
    else if (int_map == QMA6100P_MAP_INT2)
    {
        /*映射到INT2引脚，置位0x1B<3>,清除0x19<3>*/
        uint8_t reg_0x19 = 0;
        qma6100p_reg_read(0x19, &reg_0x19, 1);
        reg_0x19 &= ~(1 << 3);       /*INT1_STEP = 0*/
        qma6100p_reg_write_byte(0x19, reg_0x19);

        uint8_t reg_0x1B = 0;
        qma6100p_reg_read(0x1B, &reg_0x1B, 1);
        reg_0x1B |= (1 << 3);      /*INT2_STEP = 1*/
        qma6100p_reg_write_byte(0x1B, reg_0x1B);
    }
    else    /*int_map == QMA6100P_MAP_INT_NONE*/
    {
        /*不映射到任何引脚，同时清除0x19<3>和0x1B<3>*/
        uint8_t reg_0x19 = 0;
        qma6100p_reg_read(0x19, &reg_0x19, 1);
        reg_0x19 &= ~(1 << 3);       /*INT1_STEP = 0*/
        qma6100p_reg_write_byte(0x19, reg_0x19);

        uint8_t reg_0x1B = 0;
        qma6100p_reg_read(0x1B, &reg_0x1B, 1);
        reg_0x1B &= ~(1 << 3);      /*INT2_STEP = 0*/
        qma6100p_reg_write_byte(0x1B, reg_0x1B);
    }

    /*4.配置中断锁存模式0x21<1>*/
    /*设置为临时锁存模式：读取中断状态寄存器0x0A<3>自动清除步进中断状态位*/
    uint8_t reg_0x21;
    qma6100p_reg_read(0x21, &reg_0x21, 1);
    reg_0x21 &= ~(1 << 1);  /*INT1/INT2_latch = 0(临时锁存)*/
    qma6100p_reg_write_byte(0x21, reg_0x21);
}

static uint8_t qma6100p_config(void)
{
    static uint8_t id_data[2];

    /*qma6100p初始化序列*/
    qma6100p_reg_write_byte(QMA6100P_REG_RESET, QMA6100P_RESET);    /*复位寄存器值*/
    vTaskDelay(20);
    qma6100p_reg_write_byte(QMA6100P_REG_RESET, QMA6100P_RESET_END);/*复位后重置寄存器状态*/
    vTaskDelay(30);

    /*读取设备id，正常是0x90*/
    qma6100p_reg_read(QMA6100P_REG_CHIP_ID, id_data, 1);
    printf("id_data: %d\n", id_data[0]);

    /*两步唤醒传感器，先给模拟部分上电（ACTIVE），再开启数字内核（ACTIVE_DIGITAL）。这是 QMA6100P 的标准唤醒流程。*/
    qma6100p_reg_write_byte(QMA6100P_REG_POWER_MANAGE, QMA6100P_ACTIVE);    /*开启工作模式*/
    qma6100p_reg_write_byte(QMA6100P_REG_POWER_MANAGE, QMA6100P_ACTIVE_DIGITAL);
    qma6100p_reg_write_byte(0x4a, 0x20);
    qma6100p_reg_write_byte(0x56, 0x01);
    qma6100p_reg_write_byte(0x5f, 0x80);
    vTaskDelay(1);
    qma6100p_reg_write_byte(0x5f, 0x00);
    vTaskDelay(10);

    qma6100p_reg_write_byte(QMA6100P_REG_RANGE, QMA6100P_RANGE_8G);
    qma6100p_reg_write_byte(QMA6100P_REG_BW_ODR, QMA6100P_BW_100);
    qma6100p_reg_write_byte(QMA6100P_REG_POWER_MANAGE, QMA6100P_MCLK_51_2K | QMA6100P_ACTIVE);

    /*本次实验中断暂未用到*/
    //qma6100p_reg_write_byte(0x21, 0x03);    /*0x21中断功能寄存器，0x03使能步进中断输出*/

    //qma6100p_step_int_config(QMA6100P_MAP_INT1, 1); /*映射中断引脚，并使能中断*/

    if (id_data[0] == 0x90)
    {
        ESP_LOGI("qma6100p", "qma6100p read id success!");
        return 0;   /*qma6100p正常*/
    }
    else
    {
        ESP_LOGE("qma6100p", "qma6100p read id fail!");
        return 1;   /*qma6100p失败*/
    }
}

void qma6100p_init(i2c_obj_t self)
{
    if (self.init_flag == ESP_FAIL)
    {
        qma6100p_i2c_master = iic_init(I2C_NUM_0);
    }
    else
    {
        qma6100p_i2c_master = self;
    }

    while (qma6100p_config())
    {
        ESP_LOGE("qma6100p", "qma6100p init fail!!!");
        vTaskDelay(500);
    }
}

void qma6100p_read_raw_xyz(int16_t data[3])
{
    uint8_t databuf[6] = {0};

    qma6100p_reg_read(QMA6100P_REG_XOUTL, databuf, 6);

    data[0] = QMA6100P_REG_ACC_VAL(databuf[0], databuf[1]);
    data[1] = QMA6100P_REG_ACC_VAL(databuf[2], databuf[3]);
    data[2] = QMA6100P_REG_ACC_VAL(databuf[4], databuf[5]);
}

void qma6100p_read_acc_xyz(float accdata[3])
{
    int16_t rawdata[3];

    qma6100p_read_raw_xyz(rawdata);

    accdata[0] = (float)rawdata[0] / QMA6100P_SENSITIVITY_8G * M_G;
    accdata[1] = (float)rawdata[1] / QMA6100P_SENSITIVITY_8G * M_G;
    accdata[2] = (float)rawdata[2] / QMA6100P_SENSITIVITY_8G * M_G;
}

void qma6100p_read_rawdata(qma6100p_rawdata_t* rawdata)
{
    float accdata[3];
    float acc_normal;

    qma6100p_read_acc_xyz(accdata);

    rawdata->acc_x = accdata[0];
    rawdata->acc_y = accdata[1];
    rawdata->acc_z = accdata[2];

    rawdata->acc_g = sqrtf(rawdata->acc_x * rawdata->acc_x +
                           rawdata->acc_y * rawdata->acc_y +
                           rawdata->acc_z * rawdata->acc_z);

    /*计算姿态角（Pitch & Roll）*/
    /*常用公式，利用重力加速度在传感器三轴上的分量来计算倾角*/
    /**Pitch:绕Y轴(横向)的旋转（俯仰角）
     * 公式：Pitch = atan2(Ax, Az) * 180 / PI (Ax, Az单位为m/s²)
     * atan2f(x, z) 比 atan(x/z) 更安全，能处理 z=0 的情况，并能区分象限。
     * atan2f返回值能覆盖-180°~180°全范围
    */
    rawdata->pitch = atan2f(rawdata->acc_x, rawdata->acc_z) * RAD_TO_DEG;

    /*Roll:绕X轴(竖向)的旋转（翻滚角）*/
    /*为避免垂直时Az=0导致的除零错误，使用合加速度acc_g进行归一化*/
    acc_normal = rawdata->acc_y / rawdata->acc_g;
    /*限制归一化值在[-1,1]区间内，防止因浮点误差导致asin参数越界*/
    if (acc_normal > 1.0f) acc_normal = 1.0f;
    if (acc_normal < -1.0f) acc_normal = -1.0f;
    rawdata->roll = asinf(acc_normal) * RAD_TO_DEG; /*asin只能返回 -90°~90° 的角度*/

    /*注：以上涉及的计算有点复杂了，不做过多了解*/
}
