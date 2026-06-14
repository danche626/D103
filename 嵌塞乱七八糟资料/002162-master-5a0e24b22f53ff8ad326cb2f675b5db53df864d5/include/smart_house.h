#ifndef _SMART_HOUSE_H
#define _SMART_HOUSE_H

#include "lz_hardware.h"
#include "ohos_init.h"
#include "los_swtmr.h"
#include "los_sem.h"
#include "los_event.h"
#include <string.h>
#include "lcd.h"
#include "Send_Printf.h"
#include "sensor.h"
#include "robot_sg90.h"
#include "TTP229.h"
#include "TM1650.h"
#include "picture.h"
#include "wifi_task.h"

#define LOG_SH_TAG                 "smart house"
#define LOG_SH_TAG_INFO            "["LOG_SH_TAG"          %4d] "
#define LOG_SH_TAG_NET_DATA        "["LOG_SH_TAG" NET DATA %4d] "
#define LOG_SH_TAG_SOUND           "["LOG_SH_TAG" SOUND    %4d] "
#define LOG_SH_TAG_UART            "["LOG_SH_TAG" UAERT    %4d] "
#define LOG_SH_TAG_SWTMR           "["LOG_SH_TAG" SWTMR    %4d] "


#define  DOOR_PWD               0X1234                           // 开门密码
#define  SMOKE_CALIB            0XA7C2                           // 校准mq2键盘输入
#define  SMOKE_FINISH           0XC17A                           // 跳过预热

#define  LIGHT_SET_VALUE_MIN    20                               // 亮度设置最小值
#define  SMOKE_SET_VALUE_MIN    450                              // 烟雾设置最小值
#define  LUMINACE_DARK_VALUE    20                               // lux hb1750结果认为天黑了的阈值
#define  SMOKE_VALUE            3500                             // ppm mq2烟雾报警阈值

#define SMOKE_STEP_VALUE        500                              // ppm设置步距
#define LIGHT_STEP_VALUE        100                              // 亮度设置步距


#define ONLINE                  "Online\r\n"                     // 在线
#define OFFLINE                 "Offline\r\n"                    // 离线
#define OPEN_DOOR               "Open the door\r\n"              // 开门
#define CLOSE_DOOR              "Close the door\r\n"             // 关门
#define OPEN_WINDOW             "Open the window\r\n"            // 开窗
#define CLOSE_WINDOW            "Close the window\r\n"           // 关窗
#define OPEN_GARAGE_DOOR        "Open the garage door\r\n"       // 开车库门
#define CLOSE_GARAGE_DOOR       "Close the garage door\r\n"      // 关车库门
#define STR_MUTE                "Mute\r\n"                       // 静音
#define STR_EXITMUTE            "ExitMute\r\n"                   // 取消静音
#define OPEN_LAMP               "Open lamp\r\n"                  // 打开灯
#define CLOSE_LAMP              "Close lamp\r\n"                 // 关闭灯
#define OPEN_ALL                "Open All\r\n"                   // 所有打开
#define CLOSE_ALL               "Close All\r\n"                  // 所有关闭
#define SKIP_HEAT               "Skip_heat\r\n"                  // 跳过预热
#define PPM_CALIB               "Ppm_calib\r\n"                  // 重新校准
#define PPMUP                   "PpmUp\r\n"                      // 增加烟雾报警阈值
#define PPMDOWN                 "PpmDown\r\n"                    // 减少烟雾报警阈值
#define LIGHTUP                 "LightUp\r\n"                    // 增加亮度阈值
#define LIGHTDOWN               "LightDown\r\n"                  // 减少亮度阈值

#define CMPSTRLEN(str)          str, sizeof(str)


#define EVENT_WAIT              0x00000001
 
 
#define TIM_BASE_TO_MS          10                               // 基础定时器时间10MS
#define TIM_UART_REV_MS         50                               // 串口接收50MS
#define TIM_REPORT_STATE_MS     1000                             // 状态报告1000MS
#define TIM_UPDATE_SU03T_MS     5000                             // 更新数据5000MS

#define STR_SH_SWTMR_BASE           "sh_base"                    //
#define STR_SH_SWTMR_UART_REV       "sh_uart_rev"                // 
#define STR_SH_SWTMR_REPORT_STATE   "sh_report_state"            // 
#define STR_SH_SWTMR_UPDATE_SU03T   "sh_update_su03t"            //
 
#define TIM_MS_COUNT(x)         (x/TIM_BASE_TO_MS) 
#define DELAY_TIME_MS           TIM_MS_COUNT(5000)               // 5s
#define SMOKEINIT_TIME_MS       TIM_MS_COUNT(38000)              // 38s

typedef enum
{
    E_FIFO_TYPE_UART,
    E_FIFO_TYPE_SOUND,
    E_FIFO_TYPE_MAX,
}E_FIFO_TYPE;

const char *m_fifo_str[E_FIFO_TYPE_MAX] =
{
    [E_FIFO_TYPE_UART]         = "uart",  
    [E_FIFO_TYPE_SOUND]        = "sound",  
};

#ifdef  FIFO_MAXSIZE
    #undef  FIFO_MAXSIZE
#endif
#define FIFO_MAXSIZE      5
#define TXT_BUF_LEN       128
typedef char char_txt[TXT_BUF_LEN];

typedef enum
{
    E_SOUND_TYPE_OPEN_DOOR  = 1,
    E_SOUND_TYPE_CLOSE_DOOR,
    E_SOUND_TYPE_OPEN_WINDOW,
    E_SOUND_TYPE_CLOSE_WINDOW,
    E_SOUND_TYPE_OPEN_GARAGE_DOOR,
    E_SOUND_TYPE_CLOSE_GARAGE_DOOR,
    E_SOUND_TYPE_OPEN_ALL,
    E_SOUND_TYPE_CLOSE_ALL,
    E_SOUND_TYPE_PSWD_ERR,
    E_SOUND_TYPE_PSWD_NOP,
    E_SOUND_TYPE_ALARM_FIRE,
    E_SOUND_TYPE_ALARM_SMOKE,
    E_SOUND_TYPE_RAIN_CLOSE_WINDOW,
    E_SOUND_TYPE_OPEN_LAMP,
    E_SOUND_TYPE_CLOSE_LAMP,

    E_SOUND_TYPE_RINGTONE  = 50,
    E_SOUND_TYPE_ALARM,
    E_SOUND_TYPE_WELCOME,

    E_SOUND_TYPE_UPVAL     = 80,
    E_SOUND_TYPE_RESET,

    E_SOUND_TYPE_SKIP_HEAT = 120,
    E_SOUND_TYPE_PPM_CALIB,
    E_SOUND_TYPE_PPMUP,
    E_SOUND_TYPE_PPMDOWN,
    E_SOUND_TYPE_LIGHTUP,
    E_SOUND_TYPE_LIGHTDOWN,

    E_SOUND_MAX,

}SOUND_TYPE_E;

typedef struct
{
    unsigned char Door_Control;
    unsigned char Window_Control;
    unsigned char Car_Control;
    unsigned char Fan_Control;
    unsigned char voice_door;
    unsigned char voice_window;
    unsigned char voice_car;

} Control_data_t; //舵机控制

typedef struct
{
    uint32_t  door;
    uint32_t  car_door;
    uint32_t  window;
    uint32_t  smoke;
    uint32_t  lamp;
    uint32_t  no_key;
    uint32_t  voice_window;
    uint32_t  voice_car;
}delay_ctrl_t;

typedef struct
{
    uint8_t   len;
    char_txt  txt;
}uart_rev_t;

#define SU03T_UART_MSG_HEAD  0X55AA
#define SU03T_UART_MSG_TAIL  0XAA55

typedef struct 
{
    uint8_t       _head[0];
    uint16_t      head;
    uint8_t       msg_num;
    uint8_t       _tail[0];
    uint16_t      tail;
} __attribute__((packed))su03t_uart_msg_t;

typedef struct 
{
    uint8_t       _head[0];
    uint16_t      head;
    uint8_t       msg_num;
    double        tmp;
    double        lum;
    double        light;
    double        ppm;
    uint8_t       _tail[0];
    uint16_t      tail;
} __attribute__((packed))su03t_uart_upmsg_t;

typedef struct
{
    uint32_t         delay;
    su03t_uart_msg_t txt;
} __attribute__((packed))sound_play_t;

typedef struct 
{
    int            type;
    int            init;
    UINT32         muxlock;
    uint16_t       offset_read;
    uint16_t       offset_write;
    uint32_t       buffer_size;
    uint8_t       *buffer;
}fifo_head_t;

/* 定义先进先出队列 */
typedef struct 
{
    fifo_head_t     head;
    uart_rev_t     *buffer; //[FIFO_MAXSIZE]
}fifo_uart_t;

/* 定义先进先出队列 */
typedef struct 
{
    fifo_head_t     head;
    sound_play_t   *buffer;//[FIFO_MAXSIZE]
}fifo_sound_t;

typedef union 
{
    fifo_head_t     head;
    fifo_uart_t     uart;
    fifo_sound_t    sound;
}fifo_u;

static inline void FifoInit(void *f)
{
    fifo_u      *fu   = (fifo_u*)f;
    fifo_head_t *fifo = &fu->head;
    int          size = FIFO_MAXSIZE*fifo->buffer_size;

    fifo->init = false;
    if(size == 0)
    {
        printf(LOG_SH_TAG_INFO"[%s]fifo %s buf no space\n", __LINE__, __FUNCTION__, m_fifo_str[fifo->type]);
        return;
    } 

    fifo->offset_read = fifo->offset_write = 0;
    if (LOS_MuxCreate(&fifo->muxlock) != LOS_OK)
    {
        printf(LOG_SH_TAG_INFO"[%s]fifo %s Falied to create mux\n", __LINE__, __FUNCTION__, m_fifo_str[fifo->type]);
        return;
    }

    printf(LOG_SH_TAG_INFO"fifoaddr:%#x %s count:%d buf size:%d\r\n", __LINE__, f, m_fifo_str[fifo->type], FIFO_MAXSIZE, size);
    fifo->buffer = calloc(1, size);
    if(fifo->buffer == NULL)
    {
        printf(LOG_SH_TAG_INFO"[%s]fifo %s Falied to memory calloc\n", __LINE__, __FUNCTION__, m_fifo_str[fifo->type]);
        return;
    }
    fifo->init = true;
}

static inline void FifoPut(void *f, void *msg)
{
    fifo_u      *fu   = (fifo_u*)f;
    fifo_head_t *fifo = &fu->head;

    if(fifo->init != true) return;

    if (fifo->init && LOS_MuxPend(fifo->muxlock, LOS_WAIT_FOREVER) == LOS_OK)
    {
        if(fifo->offset_read != (fifo->offset_write + 1) % FIFO_MAXSIZE)
        {
            // memset(&fifo->buffer[fifo->offset_write*fifo->buffer_size], 0, fifo->buffer_size);
            memcpy(&fifo->buffer[fifo->offset_write*fifo->buffer_size], msg, fifo->buffer_size);
            fifo->offset_write = (fifo->offset_write + 1) % FIFO_MAXSIZE;
        }
        else
        {
            printf(LOG_SH_TAG_INFO"fifo type %s full\r\n", __LINE__, m_fifo_str[fifo->type]);
        }
        LOS_MuxPost(fifo->muxlock);
    }
}

static inline int FifoGet(void *f, void *msg)
{
    fifo_u      *fu   = (fifo_u*)f;
    fifo_head_t *fifo = &fu->head;

    if(fifo->init != true) return 0;

    if (fifo->init && LOS_MuxPend(fifo->muxlock, LOS_WAIT_FOREVER) == LOS_OK)
    {
        if (fifo->offset_read == fifo->offset_write)
        {
            LOS_MuxPost(fifo->muxlock);
            return -2;
        }
        memcpy(msg, &fifo->buffer[fifo->offset_read*fifo->buffer_size], fifo->buffer_size);
        fifo->offset_read = (fifo->offset_read + 1) % FIFO_MAXSIZE;
        LOS_MuxPost(fifo->muxlock);
        return 1;
    }
    return 0;
}


typedef struct
{    
    uint16_t                    w;
    uint16_t                    h;
    uint8_t                     dir;                     //显示方向 设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏
    uint16_t                    f_color;                 //字体颜色
    uint16_t                    b_color;                 //背景颜色
    uint16_t                    f_size;                  //字体尺寸
    uint8_t                     start_line;              //显示内容起始行
}lcd_param_t;


typedef enum
{
    THREAD_ID_ST,
    THREAD_ID_READ_SD,
    THREAD_ID_SCREEN,
    THREAD_ID_SOUND,
    THREAD_ID_WIFI,
    THREAD_ID_REV_NETD,
    THREAD_ID_MAX,
}thread_id_e;

typedef uint32_t athread_id[THREAD_ID_MAX];

typedef struct
{
    int                           onoff;

}door_windows_t;

typedef struct
{
    uint8_devnum                  devnum;                         //设备号
    athread_id                    thread_id;
    EVENT_CB_S                    event;
    char                         *sound_str[E_SOUND_MAX];
    swtmr_t                       swtmr;                          //软件定时器

    uint32_t                      input_count;                    //矩阵键盘输入计数
    uint32_t                      password;                       //矩阵键盘输入密码
    uint8_t                       key_val[10];                    //按键值用于数码管显示

    door_windows_t                door;                           //门窗用于语音播放控制
    door_windows_t                cardoor;
    door_windows_t                windows;

    uint32_t                      report_flag;                    //是否上报状态标志 1上报
    uint32_t                      smoke_alarm_threshold;          //报警阈值
    uint32_t                      brightness_threshold;           //亮度阈值

    lcd_param_t                  *lcd;                            //屏幕参数
    delay_ctrl_t                 *dctrl;                          //延时控制
    Sensor_data_t                *sensor_data;                    //传感器数据
    Control_data_t               *control_data;                   //
}smart_house_t;


void SU03T_Ctrl(uint8_t read_buf);
void Sound_play(SOUND_TYPE_E msg_num, uint32_t delay_ms, char *txt);
void send_to_su03t_msg(SOUND_TYPE_E msg_num, uint32_t delay_ms);







#endif
