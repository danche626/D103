
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "ohos_init.h"
#include "lz_hardware.h"
#include "sensor.h"

#define IA_I2C0      0
#define BH1750_ADDR  0x23
#define SHT30_ADDR   0x44
#define CAL_PPM      20        //校准环境中PPM值
#define RL           1         //RL阻值
static float m_r0 = 10;        //元件在干净空气中的阻值

/***************************************************************
 * 名		称: iss_adc_dev_s
 * 说    明：ADC设备结构体
 ***************************************************************/
typedef struct
{
    int init;    //初始化状态
    int port;    //端口
    Pinctrl adc; // io总线
} iss_adc_dev_s;
typedef iss_adc_dev_s iss_mq2_dev_s;
typedef iss_adc_dev_s light_dev_s;

/***************************************************************
 * 名		称: GRF_SOC_CON29
 * 说    明：GRF_SOC_CON29寄存器结构体
 ***************************************************************/
typedef union
{
    uint32_t value;
    struct
    {
        uint16_t grf_saradc_ana_reg_low : 4;
        uint16_t grf_saradc_vol_sel : 1;
        uint16_t grf_saradc_ana_reg_high : 11;
        struct
        {
            uint16_t grf_saradc_ana_reg_low : 4;
            uint16_t grf_saradc_vol_sel : 1;
            uint16_t grf_saradc_ana_reg_high : 11;
        } rw;
    };
} GRF_SOC_CON29;
// #define GRF_SOC_CON29_OFFSET 0X0274 //偏移值

static iss_mq2_dev_s m_iss_mq2 = {
    .init = 0,
    .port = 4,
    .adc = {.gpio = GPIO0_PC4, .func = MUX_FUNC1, .type = PULL_NONE, .drv = DRIVE_KEEP, .dir = LZGPIO_DIR_IN, .val = LZGPIO_LEVEL_KEEP},
};

static light_dev_s m_light = {
    .init = 0,
    .port = 7,
    .adc = {.gpio = GPIO0_PC7, .func = MUX_FUNC1, .type = PULL_NONE, .drv = DRIVE_KEEP, .dir = LZGPIO_DIR_IN, .val = LZGPIO_LEVEL_KEEP},
};

static I2cBusIo m_ia_i2c0m2 = {
    .scl = {.gpio = GPIO0_PA1, .func = MUX_FUNC3, .type = PULL_NONE, .drv = DRIVE_KEEP, .dir = LZGPIO_DIR_KEEP, .val = LZGPIO_LEVEL_KEEP},
    .sda = {.gpio = GPIO0_PA0, .func = MUX_FUNC3, .type = PULL_NONE, .drv = DRIVE_KEEP, .dir = LZGPIO_DIR_KEEP, .val = LZGPIO_LEVEL_KEEP},
    .id = FUNC_ID_I2C0,
    .mode = FUNC_MODE_M2,
};

static Pinctrl touch_key              = {.gpio = GPIO0_PA4,    .func = MUX_GPIO, .type = PULL_KEEP, .drv = DRIVE_KEEP,   .dir = LZGPIO_DIR_IN,  .val = LZGPIO_LEVEL_KEEP};
static Pinctrl car_detection          = {.gpio = GPIO0_PA3,    .func = MUX_GPIO, .type = PULL_KEEP, .drv = DRIVE_KEEP,   .dir = LZGPIO_DIR_IN,  .val = LZGPIO_LEVEL_KEEP};
static Pinctrl fire_detection         = {.gpio = GPIO0_PB2,    .func = MUX_GPIO, .type = PULL_KEEP, .drv = DRIVE_KEEP,   .dir = LZGPIO_DIR_IN,  .val = LZGPIO_LEVEL_KEEP};
static Pinctrl rain_drop_detection    = {.gpio = GPIO0_PA5,    .func = MUX_GPIO, .type = PULL_UP,   .drv = DRIVE_KEEP,   .dir = LZGPIO_DIR_IN,  .val = LZGPIO_LEVEL_KEEP};
static Pinctrl infra_red_detection    = {.gpio = GPIO0_PB0,    .func = MUX_GPIO, .type = PULL_UP,   .drv = DRIVE_KEEP,   .dir = LZGPIO_DIR_IN,  .val = LZGPIO_LEVEL_KEEP};
static Pinctrl lamp_ctrl              = {.gpio = psr_io,       .func = MUX_GPIO, .type = PULL_UP,   .drv = DRIVE_KEEP,   .dir = LZGPIO_DIR_OUT, .val = LZGPIO_LEVEL_KEEP};


/***************************************************************
 * 函数名称: iss_adc_dev_init
 * 说    明: 初始化ADC设备
 * 参    数: iss_mq2_dev_s *adc
 * 返 回 值: 0 成功，1 失败
 ***************************************************************/
static uint32_t iss_adc_dev_init(iss_mq2_dev_s *adc)
{
    if (PinctrlInit(adc->adc) != 0)
    {
        printf("adc pin %d init failed\n", adc->adc.gpio);
        // return 1;
    }
    if (LzSaradcInit() != 0)
    {
        printf("saradc  %d init failed\n", adc->port);
        // return 1;
    }

    volatile GRF_SOC_CON29 *soc = (GRF_SOC_CON29 *)(0x41050000U + GRF_SOC_CON29_OFFSET);
    /*配置ADC外部参考电压grf_saradc_vol_sel=0，内部参考电压grf_saradc_vol_sel=1,rw.grf_saradc_vol_sel对应位写使能*/
    soc->rw.grf_saradc_vol_sel = 1;
    soc->grf_saradc_vol_sel    = 0;
    soc->rw.grf_saradc_vol_sel = 0;

    adc->init = 1;
    return 0;
}

/***************************************************************
* 函数名称: iss_adc_read_value
* 说    明: 读取ADC 值
* 参    数: iss_mq2_dev_s adc       ADC设备
           uint32_t *data  获取的数据
* 返 回 值: 0 成功，1 失败
***************************************************************/
static uint32_t iss_adc_read_value(iss_mq2_dev_s adc, uint32_t *data)
{
    if (adc.init == 0)
    {
        printf("ADC not init\n");
        return 1;
    }
    else if (LzSaradcReadValue(adc.port, data) != 0)
    {
        printf("ADC Read Fail\n");
        return 1;
    }
    return 0;
}

/***************************************************************
 * 函数名称: iss_get_voltage
 * 说    明: 获取电压值函数
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
static float iss_get_voltage(iss_adc_dev_s dev)
{
    unsigned int ret;
    unsigned int data;

    ret = iss_adc_read_value(dev, &data);
    if (ret != 0)
    {
        printf("ADC Read Fail\n");
    }
    return (float)data * 3.3 / 1024;
}


static __attribute__((unused)) void read_light(float *l)
{
    *l = iss_get_voltage(m_light);
}

/***************************************************************
 * 函数名称: read_mq2_ppm
 * 说    明: 获取PPM函数
 * 参    数: 无
 * 返 回 值: ppm
 ***************************************************************/
static void read_mq2_ppm(float *ppm)
{
    float voltage, rs;

    voltage = iss_get_voltage(m_iss_mq2);
    rs      = (5 - voltage) / voltage * RL;       //计算rs
    *ppm    = 613.9f * pow(rs / m_r0, -2.074f);   //计算ppm
}

/***************************************************************
 * 函数名称: e53_iss_mq2_ppm_calibration
 * 说    明: 传感器校准函数
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void e53_iss_mq2_ppm_calibration(void)
{
    float voltage = iss_get_voltage(m_iss_mq2);
    float rs = (5 - voltage) / voltage * RL;

    m_r0 = rs / pow(CAL_PPM / 613.9f, 1 / -2.074f);
    printf("m_r0:%f,Rs:%f\n", m_r0, rs);
}

/***************************************************************
 * 函数名称: init_bh1750
 * 说    明: 写命令初始化BH1750
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
static void init_bh1750(void)
{
    uint8_t send_data[1] = {0x01};
    uint32_t send_len = 1;

    LzI2cWrite(IA_I2C0, BH1750_ADDR, send_data, send_len);
}

/***************************************************************
 * 函数名称: start_bh1750
 * 说    明: 启动BH1750
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
static void start_bh1750(void)
{
    uint8_t send_data[1] = {0x10};
    uint32_t send_len = 1;

    LzI2cWrite(IA_I2C0, BH1750_ADDR, send_data, send_len);
}

/***************************************************************
 * 函数名称: init_sht30
 * 说    明: 初始化SHT30，设置测量周期
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
static void init_sht30(void)
{
    uint8_t send_data[2] = {0x22, 0x36};
    uint32_t send_len = 2;

    LzI2cWrite(IA_I2C0, SHT30_ADDR, send_data, send_len);
}

/***************************************************************
 * 函数名称: sht30_calc_RH
 * 说    明: 湿度计算
 * 参    数: u16sRH：读取到的湿度原始数据
 * 返 回 值: 计算后的湿度数据
 ***************************************************************/
static float sht30_calc_RH(uint16_t u16sRH)
{
    float humidityRH = 0;

    /*clear bits [1..0] (status bits)*/
    u16sRH &= ~0x0003;
    /*calculate relative humidity [%RH]*/
    /*RH = rawValue / (2^16-1) * 10*/
    humidityRH = (100 * (float)u16sRH / 65535);

    return humidityRH;
}

/***************************************************************
 * 函数名称: sht30_calc_temperature
 * 说    明: 温度计算
 * 参    数: u16sT：读取到的温度原始数据
 * 返 回 值: 计算后的温度数据
 ***************************************************************/
static float sht30_calc_temperature(uint16_t u16sT)
{
    float temperature = 0;

    /*clear bits [1..0] (status bits)*/
    u16sT &= ~0x0003;
    /*calculate temperature [℃]*/
    /*T = -45 + 175 * rawValue / (2^16-1)*/
    temperature = (175 * (float)u16sT / 65535 - 45);

    return temperature;
}

/***************************************************************
* 函数名称: sht30_check_crc
* 说    明: 检查数据正确性
* 参    数: data：读取到的数据
            nbrOfBytes：需要校验的数量
            checksum：读取到的校对比验值
* 返 回 值: 校验结果，0-成功 1-失败
***************************************************************/
static uint8_t sht30_check_crc(uint8_t *data, uint8_t nbrOfBytes, uint8_t checksum)
{
    uint8_t crc = 0xFF;
    uint8_t bit = 0;
    uint8_t byteCtr;
    const int16_t POLYNOMIAL = 0x131;

    /*calculates 8-Bit checksum with given polynomial*/
    for (byteCtr = 0; byteCtr < nbrOfBytes; ++byteCtr)
    {
        crc ^= (data[byteCtr]);
        for (bit = 8; bit > 0; --bit)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }

    if (crc != checksum)
        return 1;
    else
        return 0;
}

static void read_bh1750(float *l)
{
    uint8_t recv_data[2] = {0};
    uint32_t receive_len = 2;

    if(l == NULL) return;
    LOS_MuxPend(g_i2c_muxlock, LOS_WAIT_FOREVER);
    start_bh1750();
    LOS_Msleep(180);
    LzI2cRead(IA_I2C0, BH1750_ADDR, recv_data, receive_len);
    LOS_MuxPost(g_i2c_muxlock);

    *l = (float)(((recv_data[0] << 8) + recv_data[1]) / 1.2);
}

static void read_sht30(float *t, float *h)
{
    uint8_t   rc;
    uint8_t   data[3];
    uint16_t  tmp;
    uint32_t  send_len     = 2;
    uint32_t  receive_len  = 6;
    uint8_t   send_data[2] = {0xE0, 0x00};
    uint8_t   SHT30_Data_Buffer[receive_len];
    
    if(t == NULL || h == NULL) return;
    memset(SHT30_Data_Buffer, 0, receive_len);
    LOS_MuxPend(g_i2c_muxlock, LOS_WAIT_FOREVER);
    LzI2cWrite(IA_I2C0, SHT30_ADDR, send_data, send_len);
    LzI2cRead(IA_I2C0, SHT30_ADDR, SHT30_Data_Buffer, receive_len);
    LOS_MuxPost(g_i2c_muxlock);

    /*check temperature*/
    data[0] = SHT30_Data_Buffer[0];
    data[1] = SHT30_Data_Buffer[1];
    data[2] = SHT30_Data_Buffer[2];
    rc = sht30_check_crc(data, 2, data[2]);
    if (!rc)
    {
        tmp = ((uint16_t)data[0] << 8) | data[1];
        *t = sht30_calc_temperature(tmp);
    }

    /*check humidity*/
    data[0] = SHT30_Data_Buffer[3];
    data[1] = SHT30_Data_Buffer[4];
    data[2] = SHT30_Data_Buffer[5];
    rc = sht30_check_crc(data, 2, data[2]);
    if (!rc)
    {
        tmp = ((uint16_t)data[0] << 8) | data[1];
        *h = sht30_calc_RH(tmp);
    }
}


void lamp_on_off(int v)
{
    LzGpioSetVal(lamp_ctrl.gpio, v==LZGPIO_LEVEL_LOW?LZGPIO_LEVEL_LOW:LZGPIO_LEVEL_HIGH);
}

void sensor_io_init(void)     
{
    /*初始化I2C*/
    if (I2cIoInit(m_ia_i2c0m2) != LZ_HARDWARE_SUCCESS)
    {
        printf("init I2C I2C0 io fail\n");
    }
    /*I2C时钟频率100K*/
    if (LzI2cInit(IA_I2C0, 100000) != LZ_HARDWARE_SUCCESS)
    {
        printf("init I2C I2C0 fail\n");
    }
    iss_adc_dev_init(&m_iss_mq2);
#ifdef VERSION_0    
    iss_adc_dev_init(&m_light);
#endif
    PinctrlInit(touch_key);
    PinctrlInit(car_detection);
    PinctrlInit(fire_detection);
    PinctrlInit(rain_drop_detection);
    PinctrlInit(infra_red_detection);
    PinctrlInit(lamp_ctrl);
    init_bh1750();
    init_sht30();
}

void sensor_read_data(Sensor_data_t *pData)
{
    // bh1750传感器读取
    read_bh1750(&pData->luminance);

    // sht30传感器读取
    read_sht30(&pData->temperature, &pData->humidity);

    // mq2传感器读取
    read_mq2_ppm(&pData->ppm);

    // 人体红外检测
    LzGpioGetVal(infra_red_detection.gpio, (LzGpioValue *)&pData->infra_red_flag);

    // 雨滴传感器读取
    LzGpioGetVal(rain_drop_detection.gpio, (LzGpioValue *)&pData->rain);

    // 循迹传感器读取
    LzGpioGetVal(car_detection.gpio, (LzGpioValue *)&pData->car_flag);

    // 火焰传感器读取
    LzGpioGetVal(fire_detection.gpio, (LzGpioValue *)&pData->fire_flag);

    // 触摸按键状态读取
    LzGpioGetVal(touch_key.gpio, (LzGpioValue *)&pData->touch_key); 
}
