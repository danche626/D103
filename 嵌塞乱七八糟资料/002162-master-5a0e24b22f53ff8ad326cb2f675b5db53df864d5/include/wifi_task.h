#ifndef _WIFI_TASK_H
#include "config_network.h"
#include "com_msg.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/stats.h"
#include "lwip/inet_chksum.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "wifi_device.h"

#define SERVER_IP    "192.168.2.167"
#define SERVER_IP1   "192.168.137.66"

#define LOCAL_PORT   8888
#define REMOTE_PORT  9999
#define REMOTE_PORT1 6668


#define TCP_TIME_OUT_S   30

#define BUFF_LEN        (sizeof(app_msg_data_t)+10)

/* 定义先进先出队列 */
#ifdef  FIFO_MAXSIZE
    #undef  FIFO_MAXSIZE
#endif
#define FIFO_MAXSIZE      10
typedef struct 
{
    bool            init;
    UINT32          muxlock;
    uint32_t        offset_read;
    uint32_t        offset_write;
    app_msg_data_t *buffer;
}fifo_s;

typedef enum
{
    WIFI_MODE_AP = 0,
    WIFI_MODE_STA,
    WIFI_MODE_DEFAULT = WIFI_MODE_STA,
}wifi_mode_e;

typedef enum
{
    UDP_TASK_STATE_INIT,
    UDP_TASK_STATE_RUN,
    UDP_TASK_STATE_STOP,
    UDP_TASK_STATE_END,

}udp_task_state_e;


typedef struct
{
    data_wifi_type_e              wifi;
    data_wifi_type_e              client_pc;
    data_wifi_type_e              client_ph;
    data_wifi_type_e              server;

}wifi_link_state_t;

typedef int (*ptcp_read_msg)(void *);
typedef void (*ptcp_write_msg)(void *);
typedef int (*pget_link_state)(void *);
typedef void (*pudp_task_start)(void *);
typedef void (*pwifi_msg_send_to_lcd)(void *wifi, data_wifi_type_e);
typedef void (*pwifi_queue_rev)(uint32_t);


typedef struct
{
    WifiLinkedInfo                linkedinfo;
    pget_link_state               get_link_state_fn;
}sta_mode_t;


typedef struct
{
    pget_link_state               get_link_state_fn;
}ap_mode_t;

typedef struct
{
    fifo_s                       *fifo;                           //消息队列
    uint32_t                      thread_id;
}tcp_msg_t;

typedef struct
{
    uint32_t                      thread_id;
    uint32_t                      rmthread_id;
    udp_task_state_e              thread_start_flag;              //任务运行状态
    udp_task_state_e              rmthread_start_flag;            
    int32_t                       socket_fd;
    int32_t                       client_fd;
    uint8_t                       time_out_flag;                  //连接超时标志1超时
    struct timeval                timeout;                        //
    struct sockaddr_in            sockaddr;

    ptcp_read_msg                 tcp_read_msg_fn;                //接收任务
    ptcp_write_msg                tcp_write_msg_fn;               //消息发送
}tcp_mode_t;

typedef struct
{
    wifi_mode_e                   mode;                           //wifi 模式
     
    ap_mode_t                    *ap;
    sta_mode_t                   *sta;
    tcp_mode_t                   *client_pc;
    tcp_mode_t                   *client_ph;
    tcp_mode_t                   *server;
    tcp_msg_t                    *read;
    tcp_msg_t                    *write_pc;
    tcp_msg_t                    *write_ph;

    uint32_t                      local_ip;
    uint16_t                      local_port;
    uint32_t                      remote_ip;
    uint16_t                      remote_port;
    uint32_t                      gw;
    uint32_t                      netmask;

    uint32_t                      relthread_id;
    bool                          relink_flag;                    //重连wifi
    bool                          link_stop;                      //停止wifi连接

    pudp_task_start               udp_task_start_fn;              //udp 任务启动
    pwifi_msg_send_to_lcd         msg_send_to_lcd_fn;             //lcd显示消息
    pwifi_queue_rev               wifi_queue_rev_fn;
    
    wifi_link_state_t             link_state;                     //连接状态
    app_msg_t                     lcd_msg;
    swtmr_t                       swtmr;                          //软件定时器
    
}  __attribute__((aligned(4)))wifi_task_t;



#define STR_WIFI_SWTMR_LINK_TEST          "wifi_link_test"             //连接检测

#define WIFI_SWTMR_LINK_TEST_PERIOD_MS     15000                       //自动检测间隔


int get_sta_link(void *wifi);

void wifi_process(void *args);

#define ADDRBUF_LEN   16
#define __inet_ntoa(addr, addrbuf)  _inet_ntoa((const struct in_addr *)&(addr), (char*)addrbuf)
static inline char *_inet_ntoa(const struct in_addr *addr, char *addrbuf)
{
    if(addrbuf == NULL) return addrbuf;
    memset(addrbuf, 0 , ADDRBUF_LEN);
    char *ip = inet_ntoa(*addr);
    strncpy(addrbuf, ip, ADDRBUF_LEN);
    return addrbuf;
}


#define  RESPONE_SEND_TYPE_BROADCAST      0      // 广播响应固定端口9999响应
#define  RESPONE_SEND_TYPE_SINGLE_IP      1      // 根据接收到数据的ip固定端口9999响应
#define  RESPONE_SEND_TYPE_SINGLE_IP_PORT 2      // 根据接收到数据的ip和端口响应

#define  RESPONE_SEND_TYPE                RESPONE_SEND_TYPE_BROADCAST







#endif