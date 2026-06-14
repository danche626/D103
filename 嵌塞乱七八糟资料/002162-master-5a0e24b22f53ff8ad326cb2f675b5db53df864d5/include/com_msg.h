#ifndef __COM_MSG_H

#include "los_task.h"
#include "los_queue.h"
#include "swtmr.h"
#include "securec.h"
#include "lz_hardware.h"
#include "log.h"

/* define   ------------------------------------------------------------------*/
typedef int8_t                  S8 ;  //!< 8-bit signed integer.
typedef uint8_t                 U8 ;  //!< 8-bit unsigned integer.
typedef int16_t                 S16;  //!< 16-bit signed integer.
typedef uint16_t                U16;  //!< 16-bit unsigned integer.
typedef uint16_t                le16_t;
typedef uint16_t                be16_t;
typedef int32_t                 S32;  //!< 32-bit signed integer.
typedef uint32_t                U32;  //!< 32-bit unsigned integer.
typedef uint32_t                le32_t;
typedef uint32_t                be32_t;
typedef int64_t                 S64;  //!< 64-bit signed integer.
typedef uint64_t                U64;  //!< 64-bit unsigned integer.
typedef float                   F32;  //!< 32-bit floating-point number.
typedef double                  F64;  //!< 64-bit floating-point number.
typedef uint32_t                iram_size_t;

#define Swap16(u16) ((U16)(((U16)(u16) >> 8) |\
                   ((U16)(u16) << 8)))
                   
#define Swap32(u32) ((U32)(((U32)Swap16((U32)(u32) >> 16)) |\
                   ((U32)Swap16((U32)(u32)) << 16)))
                   
#define Swap64(u64) ((U64)(((U64)Swap32((U64)(u64) >> 32)) |\
                   ((U64)Swap32((U64)(u64)) << 32)))

#define ARRAY_CNT(x)  sizeof(x)/sizeof(x[0])
typedef int (*printf_fn)(const char *fmt, ...);

static inline int pfn_nop(const char *fmt, ...)
{
    return 0;
}

// #define offsetof(type,member) ((size_t) &((type *)0)->member)
#define STOP_TASK_MSG       0X1a
#define STOP_TASK_ASK_MSG   0X2a
#define KEY_MSG             0X01


//需要与lcd_display_type_e保持同步
typedef enum
{
    E_MSG_CMD = 0,
    E_MSG_E53_C1,
    E_MSG_E53_C2,
    E_MSG_E53_C3,
    E_MSG_E53_C4,
    E_MSG_E53_C5,
    E_MSG_E53_C6,
    E_MSG_E53_C7,
    E_MSG_E53_C8,
    E_MSG_E53_C9,
    E_MSG_E53_C10,
    E_MSG_E53_C11,
    E_MSG_E53_C12,
    E_MSG_MODE,
    E_MSG_HRD,
    E_MSG_MAX,
} msg_type_e;

#define DEVNUM_LEN   12
typedef uint8_t uint8_devnum[DEVNUM_LEN];

#define SMART_HOUSE_RESPONSE_SUCCESS    0
#define SMART_HOUSE_RESPONSE_FAILURE    1
#define SMART_HOUSE_CLOSE               0
#define SMART_HOUSE_OPEN                1
#define SMART_MEDICAL_RESPONSE_SUCCESS  0
#define SMART_MEDICAL_RESPONSE_FAILURE  1

#define SMART_HOUSE_START_FALG          0XFFAA5500
typedef enum
{
    E_DEV_CMD = 0,
    E_DEV_SMART_HOUSE   = 0x0001,
    E_DEV_SMART_MEDICAL = 0x0002,

    E_DEV_MAX           = 0xffff,
} dev_type_e;

typedef enum
{
    E_SMART_HOUSE_CMD = 0,
    E_SMART_HOUSE_CMD_SEARCH_DEV               = 0x0001,
    E_SMART_HOUSE_CMD_REPORT_STATE             = 0x0002,
    E_SMART_HOUSE_CMD_OPEN_DOOR                = 0x0003,
    E_SMART_HOUSE_CMD_OPEN_CAR_DOOR            = 0x0004,
    E_SMART_HOUSE_CMD_OPEN_WINDOWS             = 0x0005,
    E_SMART_HOUSE_CMD_OPEN_LAMP                = 0x0006,
    E_SMART_HOUSE_CMD_SET_SMOKE_ALRAM_VAL      = 0x0007,
    E_SMART_HOUSE_CMD_SET_BRIGHTNESS_VAL       = 0x0008,
    E_SMART_HOUSE_CMD_RESET                    = 0x1234,
    E_SMART_HOUSE_CMD_MAX                      = 0xffff,
} smart_house_cmd_type_e;

typedef enum
{
    E_SMART_MEDICAL_CMD = 0,
    E_SMART_MEDICAL_CMD_SEARCH_DEV             = 0x0001,
    E_SMART_MEDICAL_CMD_REPORT_HRD             = 0x0002,
    E_SMART_MEDICAL_CMD_REPORT_TM              = 0x0003,
    E_SMART_MEDICAL_CMD_REPORT_ME              = 0x0004,
    E_SMART_MEDICAL_CMD_SET_TASK               = 0x0005,
    E_SMART_MEDICAL_CMD_RESET                  = 0x1234,
    E_SMART_MEDICAL_CMD_MAX                    = 0xffff,
} smart_medical_cmd_type_e;

typedef enum
{
    E_DATA_CMD = 0,
    // E_DATA_PARAM,
    E_DATA_ADCKEY_CTRL,
    E_DATA_LCD_CTRL,
    E_DATA_CTRL_LCD,
    E_DATA_CTRL_E53,
    E_DATA_CTRL_WIFI,
    E_DATA_E53_LCD,
    E_DATA_E53_TCP,
    E_DATA_WIFI_LCD,
    E_DATA_MAX,
}data_type_e;


typedef enum
{
    E_DATA_LCD_KEY,
    E_DATA_LCD_TYPE,
    E_DATA_LCD_MODE,
    E_DATA_LCD_MAX,
}data_lcd_type_e;

typedef enum
{
    E_DATA_ERROR_NO,
    E_DATA_ERROR_ID,
    E_DATA_ERROR_NO_MODEL_CHECKED,
    E_DATA_ERROR_MULT_MODEL_CHECKED,
    E_DATA_ERROR_MAX,
}data_error_type_e;

typedef enum
{
    E_DATA_WIFI_LINK,
    E_DATA_WIFI_NOLINK,
    E_DATA_WIFI_CLIENT_PC_LINK,
    E_DATA_WIFI_CLIENT_PC_NOLINK,
    E_DATA_WIFI_CLIENT_PH_LINK,
    E_DATA_WIFI_CLIENT_PH_NOLINK,
    E_DATA_WIFI_SERVER_LINK,
    E_DATA_WIFI_SERVER_NOLINK,
    E_DATA_WIFI_MAX,
}data_wifi_type_e;


//--------------------------------------------------------------------------------------
// 数据结构体
//--------------------------------------------------------------------------------------
typedef struct 
{
    int                        value;     // 1停止   
} __attribute__((packed)) msg_cmd_cmd_data_t;

typedef struct 
{
    int                        key;        
} __attribute__((packed)) msg_cmd_adckey_ctrl_data_t;

typedef struct 
{
    data_lcd_type_e            type;
    uint8_t                    value;
} __attribute__((packed)) msg_cmd_lcd_ctrl_data_t;

typedef struct 
{
    data_lcd_type_e            type;
    uint8_t                    value;
    uint8_t                    mode;
    union
    {
        int                    key;        
        msg_type_e             type;       
    } __attribute__((packed)) msg;
} __attribute__((packed)) msg_cmd_ctrl_lcd_data_t;

typedef struct 
{
    uint8_t                    cmd;   
    int                        key;     
} __attribute__((packed)) msg_cmd_ctrl_e53_data_t;

typedef struct 
{
    uint8_t                    cmd;   
    int                        key;     
} __attribute__((packed)) msg_cmd_ctrl_wifi_data_t;


typedef struct 
{
    data_wifi_type_e           type;        
} __attribute__((packed)) msg_cmd_wifi_lcd_data_t;



typedef struct 
{
    float                      luminance;   //亮度
    float                      humidity;    //湿度
    float                      temperature; //温度
    uint8_t                    led;         //led状态1 on
    uint8_t                    fan;         //风扇状态1 on
} __attribute__((packed)) msg_e53_c1_data_t;

typedef struct 
{
    int                        x;
    int                        y;
    int                        z;
    uint8_t                    state;       //井盖状态 1倾斜
} __attribute__((packed)) msg_e53_c2_data_t;

typedef struct 
{
    float                      luminance;   //亮度
    uint8_t                    led;         //led1状态1 on
} __attribute__((packed)) msg_e53_c3_data_t;

typedef struct 
{
    float                      distance;    //距离
    uint8_t                    led;         //led状态1 on
    uint8_t                    beep;        //蜂鸣器状态1 on
} __attribute__((packed)) msg_e53_c4_data_t;

typedef struct 
{
    uint8_t                    state;       //人体检测状态1 有人
    uint8_t                    led;         //灯光状态1 on
    uint8_t                    beep;        //蜂鸣器状态1 on
} __attribute__((packed)) msg_e53_c5_data_t;

typedef struct 
{
    uint16_t                   state;       //手势状态
} __attribute__((packed)) msg_e53_c6_data_t;

typedef struct 
{
    float                      ppm;         //ppm值
    uint8_t                    led;         //led状态1 on
    uint8_t                    beep;        //蜂鸣器状态1 on
} __attribute__((packed)) msg_e53_c7_data_t;

typedef struct 
{
    float                      tempe;       //温度
    uint8_t                    led;         //led状态1 on
} __attribute__((packed)) msg_e53_c8_data_t;

typedef struct 
{
    float                      heart;       //心率
    float                      spo;         //血氧
    uint8_t                    hled;        //心率led状态1 on
    uint8_t                    sled;        //血氧led状态1 on
} __attribute__((packed)) msg_e53_c9_data_t;

typedef struct 
{
    uint8_t                    fire;        //火灾检测状态1 有
    uint8_t                    led;         //led状态1 on
    uint8_t                    beep;        //蜂鸣器状态1 on
} __attribute__((packed)) msg_e53_c10_data_t;

typedef struct 
{
    float                      hrd;         //心率
} __attribute__((packed)) msg_e53_c11_data_t;

typedef struct 
{
    float                      weight;       //重量
    float                      peel_w;       //皮重
    bool                       peel_flag;
    bool                       add_flag;
    bool                       dec_flag;
} __attribute__((packed)) msg_e53_c12_data_t;

//--------------------------------------------------------------------------------------
// 消息子结构体
//--------------------------------------------------------------------------------------
typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;     
    union
    {
        msg_cmd_cmd_data_t          cmd;
        msg_cmd_adckey_ctrl_data_t  adckey_ctrl;
        msg_cmd_lcd_ctrl_data_t     lcd_ctrl;
        msg_cmd_ctrl_lcd_data_t     ctrl_lcd;
        msg_cmd_ctrl_e53_data_t     ctrl_e53;
        msg_cmd_ctrl_wifi_data_t    ctrl_wifi;
        msg_cmd_wifi_lcd_data_t     wifi_lcd;
    }  __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_cmd_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c1_data_t      data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c1_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c2_data_t      data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c2_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c3_data_t      data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c3_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c4_data_t      data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c4_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c5_data_t      data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c5_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c6_data_t      data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c6_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c7_data_t      data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c7_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c8_data_t      data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c8_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c9_data_t      data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c9_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c10_data_t     data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c10_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c11_data_t     data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c11_t;

typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    union 
    {
        int                    key;
        msg_e53_c12_data_t     data;
    } __attribute__((packed));
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_c12_t;


//MSG--------------------------------------------------------------------------------------
typedef struct 
{
    uint8_devnum               num;
    dev_type_e                 type;
} __attribute__((packed)) dev_msg_head_t;

typedef struct 
{
    uint8_devnum               num;
    dev_type_e                 type;
    uint16_t                   cmd_type;
    uint8_t                    response;
} __attribute__((packed)) dev_type_head_t;
//smart_house---------------------------------------------------------------------------------

typedef struct 
{
    dev_msg_head_t             dev;
    smart_house_cmd_type_e     cmd_type;   
    uint8_t                    response;
} __attribute__((packed)) msg_smart_house_head_t;

typedef msg_smart_house_head_t msg_smart_house_search_dev_t;

typedef struct 
{
    msg_smart_house_head_t     head;
    uint16_t                   temperature;
    uint16_t                   humidity;
    uint16_t                   luminance;
    uint8_t                    rain;
    uint8_t                    ppm;
    uint8_t                    fire;
} __attribute__((packed)) msg_smart_house_report_state_t;

typedef struct 
{
    msg_smart_house_head_t     head;
    uint8_t                    onoff;
} __attribute__((packed)) msg_smart_house_on_off_t;

typedef struct 
{
    msg_smart_house_head_t     head;
    uint32_t                   alarm_val;
} __attribute__((packed)) msg_smart_house_smoke_alarm_val_t;
typedef msg_smart_house_smoke_alarm_val_t msg_smart_house_brightness_val_t;


typedef union
{
    msg_smart_house_head_t                     head;
    msg_smart_house_search_dev_t               search_dev;
    msg_smart_house_report_state_t             report_state;
    msg_smart_house_on_off_t                   on_off;
    msg_smart_house_smoke_alarm_val_t          smoke;
    msg_smart_house_brightness_val_t           brightness;

} __attribute__((packed)) msg_smart_house_t;

//smart_medical---------------------------------------------------------------------------------
typedef struct 
{
    dev_msg_head_t             dev;
    smart_medical_cmd_type_e   cmd_type;   
    uint8_t                    response;
} __attribute__((packed)) msg_smart_medical_head_t;

typedef msg_smart_medical_head_t msg_smart_medical_search_dev_t;

typedef struct 
{
    msg_smart_medical_head_t   head;
    uint8_t                    hrd_v;
    uint8_t                    hrd;
    uint8_t                    spo_v;
    uint8_t                    spo;
} __attribute__((packed)) msg_smart_medical_report_hrd_t;

typedef struct 
{
    msg_smart_medical_head_t   head;
    uint8_t                    tm_v;
    uint16_t                   tm;
} __attribute__((packed)) msg_smart_medical_report_tm_t;

#define DRAW_NUMBER         320
typedef uint32_t uint32_hrd[DRAW_NUMBER];
typedef struct 
{
    msg_smart_medical_head_t   head;
    uint8_t                    hrd;
    uint32_t                   frame_num;
    uint32_hrd                 hrd_data;
} __attribute__((packed)) msg_smart_medical_report_me_t;

typedef struct 
{
    msg_smart_medical_head_t   head;
    uint8_t                    id;
} __attribute__((packed)) msg_smart_medical_set_task_t;

typedef union
{
    msg_smart_medical_head_t                     head;
    msg_smart_medical_search_dev_t               search_dev;
    msg_smart_medical_report_hrd_t               report_hrd;
    msg_smart_medical_report_tm_t                report_tm;
    msg_smart_medical_report_me_t                report_me;
    msg_smart_medical_set_task_t                 set_task;
    
} __attribute__((packed)) msg_smart_medical_t;

//MSG1--------------------------------------------------------------------------------------
typedef struct 
{
    uint32_t                   start_flag;
    uint16_t                   len;
    uint8_devnum               num;
    uint8_devnum               app_num;
    dev_type_e                 type;
} __attribute__((packed)) dev_msg1_head_t;

typedef struct 
{
    uint32_t                   start_flag;
    uint16_t                   len;
    uint8_devnum               num;
    uint8_devnum               app_num;
    dev_type_e                 type;
    uint16_t                   cmd_type;
    uint8_t                    response;
} __attribute__((packed)) dev_type1_head_t;
//MSG1 smart_house---------------------------------------------------------------------------------

typedef struct 
{
    dev_msg1_head_t            dev;
    smart_house_cmd_type_e     cmd_type;   
    uint8_t                    response;
} __attribute__((packed)) msg1_smart_house_head_t;

typedef msg1_smart_house_head_t msg1_smart_house_search_dev_t;


typedef struct 
{
    msg1_smart_house_head_t    head;
    uint16_t                   temperature;
    uint16_t                   humidity;
    uint16_t                   luminance;
    uint8_t                    rain;
    uint8_t                    ppm;
    uint8_t                    fire;
} __attribute__((packed)) msg1_smart_house_report_state_t;

typedef struct 
{
    msg1_smart_house_head_t     head;
    uint8_t                    onoff;
} __attribute__((packed)) msg1_smart_house_on_off_t;

typedef struct 
{
    msg1_smart_house_head_t     head;
    uint32_t                   alarm_val;
} __attribute__((packed)) msg1_smart_house_smoke_alarm_val_t;
typedef msg1_smart_house_smoke_alarm_val_t msg1_smart_house_brightness_val_t;


typedef union
{
    msg1_smart_house_head_t                     head;
    msg1_smart_house_search_dev_t               search_dev;
    msg1_smart_house_report_state_t             report_state;
    msg1_smart_house_on_off_t                   on_off;
    msg1_smart_house_smoke_alarm_val_t          smoke;
    msg1_smart_house_brightness_val_t           brightness;

} __attribute__((packed)) msg1_smart_house_t;


//--------------------------------------------------------------------------------------
// 消息结构体
//--------------------------------------------------------------------------------------
typedef struct 
{
    msg_type_e                 msg_type;
    uint16_t                   len;
    data_type_e                data_type;
    char                       tag[2];
    uint32_t                   heart[320];
    uint16_t                   crc;
} __attribute__((packed)) msg_e53_heart_t;

typedef union 
{
    msg_type_e                 type;
    msg_cmd_t                  cmd;
    msg_e53_c1_t               e53_c1;
    msg_e53_c2_t               e53_c2;
    msg_e53_c3_t               e53_c3;
    msg_e53_c4_t               e53_c4;
    msg_e53_c5_t               e53_c5;
    msg_e53_c6_t               e53_c6;
    msg_e53_c7_t               e53_c7;
    msg_e53_c8_t               e53_c8;
    msg_e53_c9_t               e53_c9;
    msg_e53_c10_t              e53_c10;
    msg_e53_c11_t              e53_c11;
    msg_e53_c12_t              e53_c12;

} __attribute__((packed)) app_msg_t;


typedef union 
{
    dev_msg_head_t             head;
    dev_type_head_t            type_head;
    msg_smart_house_t          smart_house;
    msg_smart_medical_t        smart_medical;

} __attribute__((packed)) dev_msg_t;


typedef union 
{
    dev_msg1_head_t            head;
    dev_type1_head_t           type_head;
    msg1_smart_house_t         smart_house;

} __attribute__((packed)) dev_msg1_t;




typedef union 
{
    msg_type_e                 type;
    msg_e53_heart_t            hrd;
    app_msg_t                  app_msg;
    dev_msg_t                  dev_msg;         //UDP
    dev_msg1_t                 dev_msg1;        //TCP
} __attribute__((packed)) app_msg_data_t;












uint16_t get_msg_len(app_msg_t m);
void set_msg_crc16(app_msg_t *msg);
int calc_msg_crc16(app_msg_t msg);
int get_smart_house_req_msg_len(smart_house_cmd_type_e type);
int get_smart_house_res_msg_len(smart_house_cmd_type_e type);
int get_smart_medical_req_msg_len(smart_medical_cmd_type_e type);
int get_smart_medical_res_msg_len(smart_medical_cmd_type_e type);
int get_dev_req_msg_len(dev_msg_t d);
int get_dev_res_msg_len(dev_msg_t d);
int get_smart_house_data_len(smart_house_cmd_type_e type);
int get_dev_deta_len(dev_msg1_t *d);
int get_smart_house_req_msg1_len(smart_house_cmd_type_e type);
int get_smart_house_res_msg1_len(smart_house_cmd_type_e type);
int get_dev_req_msg1_len(dev_msg1_t d);
int get_dev_res_msg1_len(dev_msg1_t d);


void get_rk2206_efuse(uint8_t *devnum);
#define  COMMON_DEV         "0123456789AB"

#define SHOW_ALL_TASK  0XFF
uint32_t task_show(int);
int create_task(unsigned int *threadID,  uint32_t size, uint8_t prio, ThreadFunc func, void *args, const char *owner);

#define TASK_PROCESS_START_LOG()   printf("\n\n==========%s start==============\n", __FUNCTION__)


//--------------------------------------------------------------------------------------
// 队列相关定义
//--------------------------------------------------------------------------------------

#define QUEUE_LEN_MAX                          10
#define QUEUE_DEFAULT_WRITE_TIME_MS            10
#define QUEUE_DEFAULT_READ_TIME_MS             200


#define WRITE_QUEUE_ERR_LOG(q) \
{\
        QUEUE_INFO_S qinfo = {0};\
        LOS_QueueInfoGet(q, &qinfo);\
        /*printf("%s write err msg cnt:%d\n", __FUNCTION__, qinfo.readableCnt);*/\
}


#define FORMAT_UINT32         "%8lu"
#define FORMAT_UINT64         "%8llu"
#define TICK32_OVER_VALUE     0XFFFFFFFF
#define TICK64_OVER_VALUE     0XFFFFFFFFFFFFFFFF

#define TICK_BIT_64           1

#if     TICK_BIT_64
typedef uint64_t uint_tick;
#define FORMAT_UINT_TICK      FORMAT_UINT64
#define TICK_OVER_VALUE       TICK64_OVER_VALUE
#else
typedef uint32_t uint_tick;
#define FORMAT_UINT_TICK      FORMAT_UINT32
#define TICK_OVER_VALUE       TICK32_OVER_VALUE
#endif

uint_tick get_current_tick(void);
uint_tick get_time_us(uint_tick start_tick);




extern void dump_hex(printf_fn pfn, char *info, uint8_t* buf, int len);


extern uint32_t g_ctrl_queue;
extern uint32_t g_lcd_queue;
extern uint32_t g_e53_queue;
extern uint32_t g_tcp_queue;
extern uint32_t g_wifi_queue;
extern uint32_t g_netrev_queue;

extern int wifi_get_connect_status_internal();











#endif
