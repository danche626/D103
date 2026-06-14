#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "los_mux.h"
#include "los_task.h"

#define  VERSION_1 

#ifdef VERSION_0
#define psr_io          GPIO0_PB1      // 光敏控制LED
#endif

#ifdef VERSION_1
#define psr_io          GPIO0_PC7      // 光敏控制LED
#endif


typedef struct
{
    float luminance;           // 亮度
    float light;               // 光敏
    float humidity;            // 湿度
    float temperature;         // 温度
    float ppm;                 // 烟雾
    int rain;                  // 雨滴
    int car_flag;              // 车库门
    int fire_flag;             // 火焰
    int infra_red_flag;        // 红外
    int lamp_flag;             // 灯
    int touch_key;             // 触摸按键

} Sensor_data_t;

void sensor_io_init();
void sensor_read_data(Sensor_data_t *pData);
void e53_iss_mq2_ppm_calibration(void);
void lamp_on_off(int v);

extern UINT32 g_i2c_muxlock;

/* 
人体红外                  GPIO0_PB0
光敏控制LED               GPIO0_PB1
火焰传感器                GPIO0_PB2

车库们检测                GPIO0_PA3      
触摸按键                  GPIO0_PA4
雨滴传感器                GPIO0_PA5
烟雾传感器                GPIO0_PC4      adc
光敏传感器                GPIO0_PC7      adc

数码管驱动TM1650 、sht30 、 bh1750、
i2c scl                 GPIO0_PA1
i2c sda                 GPIO0_PA0

TP229矩阵键盘scl         GPIO0_PA2
TP229矩阵键盘sdo         GPIO0_PB3


窗户舵机                 GPIO1_PD0       pwm
车库们舵机                GPIO0_PB4       pwm
们舵机                   GPIO0_PB5       pwm


su03t_b2 tx             GPIO0_PB6      rx
su03t_b3 rx             GPIO0_PB7      tx

lcd spi cs                  IO0_PC0
lcd spi clk                 IO0_PC1
lcd spi mosi                IO0_PC2
lcd spi res                 IO0_PC3
lcd spi dc                  IO0_PC6

*/


#endif