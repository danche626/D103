#include "lz_hardware.h"
#include "TTP229.h"
#include "los_task.h"

Pinctrl TTP229_SCL = {.gpio = GPIO0_PA2, .func = MUX_GPIO, .type = PULL_KEEP, .drv = DRIVE_LEVEL3, .dir = LZGPIO_DIR_OUT, .val = LZGPIO_LEVEL_KEEP};
Pinctrl TTP229_SDO = {.gpio = GPIO0_PB3, .func = MUX_GPIO, .type = PULL_UP, .drv = DRIVE_LEVEL3, .dir = LZGPIO_DIR_IN, .val = LZGPIO_LEVEL_KEEP};

#define TTP229_SCL_Clr() LzGpioSetVal(TTP229_SCL.gpio, LZGPIO_LEVEL_LOW)
#define TTP229_SCL_Set() LzGpioSetVal(TTP229_SCL.gpio, LZGPIO_LEVEL_HIGH)

#define TTP229_SDO_Clr() LzGpioSetVal(TTP229_SDO.gpio, LZGPIO_LEVEL_LOW)
#define TTP229_SDO_Set() LzGpioSetVal(TTP229_SDO.gpio, LZGPIO_LEVEL_HIGH)

#define TTP229_SDO_OUT() LzGpioSetDir(TTP229_SDO.gpio, LZGPIO_DIR_OUT);
#define TTP229_SDO_IN()  LzGpioSetDir(TTP229_SDO.gpio, LZGPIO_DIR_IN);

void TTP229_init()
{
    PinctrlSet(TTP229_SCL.gpio, TTP229_SCL.func, TTP229_SCL.type, TTP229_SCL.drv); //按键初始化
    LzGpioInit(TTP229_SCL.gpio);
    LzGpioSetDir(TTP229_SCL.gpio, TTP229_SCL.dir);

    PinctrlSet(TTP229_SDO.gpio, TTP229_SDO.func, TTP229_SDO.type, TTP229_SDO.drv); //按键初始化
    LzGpioInit(TTP229_SDO.gpio);
    LzGpioSetDir(TTP229_SDO.gpio, TTP229_SDO.dir);
    TTP229_SDO_OUT();
    //当TP1=0,TP2=0时，应在SDO和SCL初始化时将电平拉低
    TTP229_SCL_Clr();
    TTP229_SDO_Clr();
    LOS_Msleep(100);
    for(int i = 0; i < 20; ++i)
    {
        Touch_KeyScan();
        LOS_Msleep(20);
    }
    printf("key init end\r\n");
}

char key_val[] = " 123a456b789c*0#d";



unsigned char ttp229_read(void)
{
    static int polarity = 0;
    unsigned char num = 0, val = 0, pol;
    unsigned char i;
    
    TTP229_SDO_OUT();
    TTP229_SDO_Set();
    ToyUdelay(93);
    TTP229_SDO_Clr();
    ToyUdelay(20);
    TTP229_SDO_IN();

    for (i = 0; i < 16; i++)
    {
        //参考手册时序：1kHz < F_SCL< 512 kHz  => 1.92us < f1+f0< 1ms , f1+f0:一个高电平+一个低电平时间和
        TTP229_SCL_Set();
        ToyUdelay(100);
        TTP229_SCL_Clr();
        ToyUdelay(10);
        LzGpioGetVal(TTP229_SDO.gpio, &val);
        if (val == polarity)
        {
            num = i + 1;
            ToyUdelay(500);
        }
        else
        {
            pol++;
        }

        ToyUdelay(100);
    }
    TTP229_SDO_OUT();
    ToyMdelay(2);

    if(pol == 16)
    {
        if(val == polarity) printf("----------------------%d\r\n", val > 0? 0:1);
        polarity = val > 0? 0:1;
        
    }
    return num;
}



uint16_t Previous = 0;
uint16_t Current = 0;
uint8_t Touch_KeyScan(void)
{
    static int cnt  = 10;
    uint8_t key = key_val[0];

    if(cnt--  <= 0)
    {
        cnt = 3;
        Current = key_val[ttp229_read()];
        // printf("k:%c, %c\r\n", key, Current);
    }

    if(Current != Previous)
    {
        key = Current;
        cnt = 10;
    }

    Previous = Current;
    return key;
}
