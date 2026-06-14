#ifndef __TM1650_H__
#define __TM1650_H__

// 
typedef enum 
{
    E_DISPLAY_OFF     = 0,        // 显示开关
    E_DISPLAY_ON,        
    E_DISPLAY_TYPE_8  = 0,        // 显示类型
    E_DISPLAY_TYPE_7,       
    E_DISPLAY_LIGHT_8 = 0,        // 亮度等级
    E_DISPLAY_LIGHT_1,
    E_DISPLAY_LIGHT_2,
    E_DISPLAY_LIGHT_3,
    E_DISPLAY_LIGHT_4,
    E_DISPLAY_LIGHT_5,
    E_DISPLAY_LIGHT_6,
    E_DISPLAY_LIGHT_7,

    E_CMD_MOD       = 0x48,       // 模式命令
    E_CMD_READ_KEY  = 0x49,       // 读按键数据命令
    E_DISPLAY_ADDR1 = 0X68,       // 显存地址
    E_DISPLAY_ADDR2 = 0X6a,
    E_DISPLAY_ADDR3 = 0X6c,
    E_DISPLAY_ADDR4 = 0X6e,
}E_DISPLAY_TYPE;

#define ARRAY_CNT(x)  sizeof(x)/sizeof(x[0])


#define SET_LIGHT(d, x)         (d = ((d&0x0f) | (x<<4)))           // 设置显示亮度 0,8级亮度 1~7，1~7级亮度
#define SET_DISPLAY_TYPE(d, x)  (d = ((d&0xf7) | (x>0?0x08:0x0)))   // 设置显示类型 0段 1七段
#define SET_DISPLAY(d, x)       (d = ((d&0xfe) | (x>0?0x01:0x0)))   // 设置显示开关 0关 1开显示


int tm1650_init(uint8_t v);
int tm1650_display_single(uint8_t addr, uint8_t v);
int tm1650_display_move(uint8_t v, int flag);
int tm1650_display(uint8_t v1, uint8_t v2, uint8_t v3,uint8_t v4);
int tm1650_clear(void);



#endif