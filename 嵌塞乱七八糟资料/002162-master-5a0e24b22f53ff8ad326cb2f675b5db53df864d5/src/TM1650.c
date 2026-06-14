#include "lz_hardware.h"
#include "TM1650.h"
#include "los_mux.h"
#include "los_task.h"

extern UINT32 g_i2c_muxlock;

#define HAL_I2C_M_IGNORE_NAK (0x1 << 2) /**< if I2C_FUNC_PROTOCOL_MANGLING */

#define  TM_I2C0  0

typedef struct
{
    uint8_t       key;
    uint8_t       tab;
}display_tabe_t;

display_tabe_t  display[] = 
{
    {0x00, 0x3f},        // 0
    {0x01, 0x06},        // 1
    {0x02, 0x5b},        // 2
    {0x03, 0x4f},        // 3
    {0x04, 0x66},        // 4
    {0x05, 0x6d},        // 5
    {0x06, 0x7d},        // 6
    {0x07, 0x07},        // 7
    {0x08, 0x7f},        // 8
    {0x09, 0x6f},        // 9
    {0x0a, 0x77},        // a
    {0x0b, 0x7c},        // b
    {0x0c, 0x39},        // c
    {0x0d, 0x5e},        // d
    {0x0e, 0x79},        // e
    {0x0f, 0x71},        // f

    {'-', 0x40},         // -
    {'o', 0x3f},         // o
    {'O', 0x3f},         // O
    {'p', 0x73},         // p
    {'P', 0x73},         // P
    {'e', 0x7b},         // e
    {'E', 0x79},         // E
    {'n', 0x37},         // n
    {'N', 0x37},         // N
    {'c', 0x39},         // c
    {'C', 0x39},         // C
    {'l', 0x30},         // l
    {'L', 0x38},         // L
    {'s', 0x6d},         // s
    {'S', 0x6d},         // S
    {'r', 0x70},         // r
    {'R', 0x77},         // R
    {' ', 0x00},         // 
};



// #define SOFT_IIC

#ifdef SOFT_IIC
#include "iic.h"
static int tm1650_write(uint8_t cmd, uint8_t data)
{
    IIC_Start(); 
    IIC_Send_Byte(cmd);	    // 写命令
    if(IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1;
    }
    IIC_Send_Byte(data);    // 写数据
    IIC_Wait_Ack();
    IIC_Stop();
    
    return 0;
}
#else
static int tm1650_write(uint8_t cmd, uint8_t data)
{
    LzI2cMsg msg;

    LOS_MuxPend(g_i2c_muxlock, LOS_WAIT_FOREVER);
    msg.addr = cmd>>1;
    msg.flags = HAL_I2C_M_IGNORE_NAK;
    msg.buf = &data;
    msg.len = 1;
    int ret =  LzI2cTransfer(TM_I2C0, &msg, 1);
    LOS_MuxPost(g_i2c_muxlock);

    return ret;
}
#endif

static int tm1650_cfg_param(uint8_t param)
{
    // printf("p:0x%0x\r\n", param);
    tm1650_write(E_CMD_MOD, param);
    return 1;
}

int tm1650_init(uint8_t l)
{
    uint8_t v = 0;
    #ifdef SOFT_IIC
    IIC_Init();
    #endif
    tm1650_clear();
    SET_LIGHT(v, l);
    SET_DISPLAY_TYPE(v, E_DISPLAY_TYPE_8);
    SET_DISPLAY(v, E_DISPLAY_ON);
    tm1650_cfg_param(v);
    tm1650_display('-', '-', '-', '-');

    // for(int i = 0; i < 16; i++)
    // {
    //     tm1650_display_move(i, 2);
    //     LOS_Msleep(2000);
    // }
    return 1;
}

int tm1650_display_single(uint8_t addr, uint8_t v)
{
    tm1650_write(E_DISPLAY_ADDR1+addr*2, v);
    return 1;
}

static uint8_t tm1650_char_tab(char v)
{    
    for(int i = 0; i < ARRAY_CNT(display); ++i)
    {
        if(display[i].key == v) return display[i].tab;
    }

    return 0;
}

// //flag 0:clear, 1:no move, 2:left move, 3:right move
// int tm1650_display_move(uint8_t v, int flag)
// {
//     static uint8_t buf[8] = {0};
//     static uint8_t dis[4] = {'-', '-', '-', '-'};
//     static int index = 0, d_index = 0;

//     if(flag == 0)
//     {   
//         index = 0;
//         d_index = 0;
//         memset(buf, '-', sizeof(buf));
//     }
//     else if(flag == 1)
//     {
        
//     }
//     else if(flag == 2)
//     {
//         d_index = d_index < 4?++d_index:4;
//         buf[index] = v;
//         index = index < 7 ? ++index:7;
//         memcpy(dis, &buf[index-d_index], index-d_index+1);
//         tm1650_display(dis[3], dis[2], dis[1], dis[0]);
//     }
//     else if(flag == 3)
//     {
        
//     }
// }

int tm1650_display(uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4)
{    
    static uint8_t lv1 = 0xff, lv2 = 0xff, lv3 = 0xff, lv4 = 0xff;

    if(lv1 != v1 || lv2 != v2 || lv3 != v3 || lv4 != v4)
    {
        tm1650_display_single(0, tm1650_char_tab(v1));
        tm1650_display_single(1, tm1650_char_tab(v2));
        tm1650_display_single(2, tm1650_char_tab(v3));
        tm1650_display_single(3, tm1650_char_tab(v4));
        lv1 = v1;
        lv2 = v2;
        lv3 = v3;
        lv4 = v4;
    // printf("%x,%x,%x,%x\r\n", tm1650_char_tab(v1), tm1650_char_tab(v2), tm1650_char_tab(v3), tm1650_char_tab(v4));
    } 
    return 1;
}

int tm1650_clear(void)
{
    int v = 0;

    tm1650_cfg_param(SET_DISPLAY(v, E_DISPLAY_OFF));
    return 1;
}

