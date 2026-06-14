#include "cmsis_os2.h"
#include "los_task.h"
#include "los_mux.h"
#include "lz_hardware.h"
#include "wifi_task.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/stats.h"
#include "lwip/inet_chksum.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "wifi_device.h"

#define LOG_TAG_UDP       "udp"
#define LOG_TAG            LOG_TAG_UDP
#define LOG_TAG_INFO    "["LOG_TAG_UDP"           %4d] "
#define LOG_TAG_CLIENT  "["LOG_TAG_UDP" client    %4d] "
#define LOG_TAG_SERVER  "["LOG_TAG_UDP" server    %4d] "

extern WifiErrorCode SetApModeOn();
extern WifiErrorCode SetApModeOff();
extern WifiErrorCode SetWifiModeOn();
extern WifiErrorCode SetWifiModeOff();

extern int GetLocalWifiGw(int *const gw);
extern int GetLocalWifiNetmask(int *const netmask);
extern struct in_addr DHCPD_FindAddress();

void wifi_msg_send_to_lcd(void *wifi, data_wifi_type_e msg);
void wifi_queue_rev(uint32_t to);
int task_create_udp_client_pc_read_msg(void *wifi);
int task_create_udp_server_read_msg(void *wifi);
void udp_client_pc_write_msg(void *args);
void udp_server_write_msg(void *args);
void udp_task_start(void *wifi);


//读fifo
static fifo_s m_rfifo_msg =
{
    .init                 = false,
    .muxlock              = 0,
    .offset_read          = 0,
    .offset_write         = 0,
    .buffer               = NULL,
};

//写fifo
static fifo_s m_wfifo_pc_msg =
{
    .init                 = false,
    .muxlock              = 0,
    .offset_read          = 0,
    .offset_write         = 0,
    .buffer               = NULL,
};

//ap模式
static ap_mode_t m_ap =
{
    .get_link_state_fn      = get_sta_link,
};

//sta模式
static sta_mode_t m_sta =
{
    .linkedinfo             = {{0}},
    .get_link_state_fn      = get_sta_link,
};

//tcp客户端
static tcp_mode_t m_tcp_client_pc =
{
    .thread_id              = 0,
    .rmthread_id            = 0,
    .thread_start_flag      = UDP_TASK_STATE_INIT,
    .rmthread_start_flag    = UDP_TASK_STATE_INIT,
    .socket_fd              = 0,
    .time_out_flag          = 0,
    .sockaddr               = {0},
    .timeout                =
    {
        .tv_sec             = TCP_TIME_OUT_S,
        .tv_usec            = 0,
    },
    .tcp_write_msg_fn       = udp_client_pc_write_msg,
};

//tcp服务端
static tcp_mode_t m_tcp_server =
{
    .thread_id              = 0,
    .rmthread_id            = 0,
    .thread_start_flag      = UDP_TASK_STATE_INIT,
    .rmthread_start_flag    = UDP_TASK_STATE_INIT,
    .socket_fd              = 0,
    .client_fd              = 0,
    .time_out_flag          = 0,
    .sockaddr               = {0},
    .timeout                =
    {
        .tv_sec             = TCP_TIME_OUT_S,
        .tv_usec            = 0,
    },
    .tcp_write_msg_fn       = udp_server_write_msg,

};

//读消息
static tcp_msg_t m_read_msg =
{
    .fifo                   = &m_rfifo_msg,
    .thread_id              = 0,
};

//往pc写消息
static tcp_msg_t m_write_pc_msg =
{
    .fifo                   = &m_wfifo_pc_msg,
    .thread_id              = 0,
};


//WiFi 控制
static wifi_task_t m_wifi = 
{
    .mode                   = WIFI_MODE_DEFAULT,

    .ap                     = &m_ap,
    .sta                    = &m_sta,
    .client_pc              = &m_tcp_client_pc,
    .server                 = &m_tcp_server,
    .read                   = &m_read_msg,
    .write_pc               = &m_write_pc_msg,

    .local_ip               = 0,
    .local_port             = LOCAL_PORT,
    .remote_ip              = 0,
    .remote_port            = REMOTE_PORT,
    .gw                     = 0,
    .netmask                = 0,
    .relink_flag            = false,
    .link_stop              = false,

    .udp_task_start_fn      = udp_task_start,
    .msg_send_to_lcd_fn     = wifi_msg_send_to_lcd,
    .wifi_queue_rev_fn      = wifi_queue_rev,

    .link_state =
    {
        .wifi               = E_DATA_WIFI_NOLINK,
        .client_pc          = E_DATA_WIFI_CLIENT_PC_NOLINK,
        .client_ph          = E_DATA_WIFI_CLIENT_PH_NOLINK,
        .server             = E_DATA_WIFI_SERVER_NOLINK,
    },

    .lcd_msg                = {0},

    .swtmr = 
    {
        .cnt                = 0,
        .args               = NULL,

        .getid_fn           = swtmr_getid,
        .start_fn           = swtmr_start,
        .stop_fn            = swtmr_stop,
        .init_fn            = swtmr_init,
    }
};


static inline void FifoInit(fifo_s *fifo)
{
    fifo->offset_read = fifo->offset_write = 0;
    if (LOS_MuxCreate(&fifo->muxlock) != LOS_OK)
    {
        printf(LOG_TAG_INFO"Falied to create mux\n", __LINE__);
        return;
    }
    uint32_t size = FIFO_MAXSIZE*sizeof(app_msg_data_t);
    printf(LOG_TAG_INFO"fifoaddr:%#x count:%d buf size:%d, r:%d,w:%d\r\n", __LINE__, fifo, FIFO_MAXSIZE, size, fifo->offset_read, fifo->offset_write);
    fifo->buffer = calloc(1, size);
    if(fifo->buffer == NULL)
    {
        printf(LOG_TAG_INFO"Falied to memory calloc\n", __LINE__);
        return;
    }
    fifo->init = true;
}

static inline void FifoPut(fifo_s *fifo, app_msg_data_t *msg)
{
    if (fifo->init && LOS_MuxPend(fifo->muxlock, LOS_WAIT_FOREVER) == LOS_OK)
    {
        memmove(&fifo->buffer[fifo->offset_write], msg, sizeof(app_msg_data_t));
        fifo->offset_write = (fifo->offset_write + 1) % FIFO_MAXSIZE;
        LOS_MuxPost(fifo->muxlock);
    }
}

static inline uint32_t FifoGet(fifo_s *fifo, app_msg_data_t *msg)
{
    if (fifo->init && LOS_MuxPend(fifo->muxlock, 1000) == LOS_OK)
    {
        if (fifo->offset_read == fifo->offset_write)
        {
            LOS_MuxPost(fifo->muxlock);
            return 0;
        }
        memmove(msg, &fifo->buffer[fifo->offset_read], sizeof(app_msg_data_t));
        fifo->offset_read = (fifo->offset_read + 1) % FIFO_MAXSIZE;
        LOS_MuxPost(fifo->muxlock);
        return 1;
    }
    return -1;
}

static int GetLocalWifiIp(int * const ip)
{
    struct netif *netif = netif_find("wlan0");
    if (netif == NULL) {
        LZ_HARDWARE_LOGE(LOG_TAG, "GetLocalWifiIp: netif get fail\n");
        return ERROR_WIFI_UNKNOWN;
    }

    *ip = ip_addr_get_ip4_u32(&netif->ip_addr);
    return WIFI_SUCCESS;
}

int get_sta_link(void *wifi)
{
    wifi_task_t *w = (wifi_task_t*)wifi;
    WifiLinkedInfo *info = &w->sta->linkedinfo;

    memset(info, 0, sizeof(WifiLinkedInfo));

    if (GetLinkedInfo(info) == WIFI_SUCCESS && info->connState == WIFI_CONNECTED && info->ipAddress != 0) 
    {
        return 0;
    }

    return -1;
}

int get_sta_info(void *wifi)
{
    int ret = -1;
    wifi_task_t *w = (wifi_task_t*)wifi;
    WifiLinkedInfo *info = &w->sta->linkedinfo;
    char addrbuf[ADDRBUF_LEN] = {0};

    while (1) {
        memset(info, 0, sizeof(WifiLinkedInfo));
        if (GetLinkedInfo(info) == WIFI_SUCCESS) {
            if (info->connState == WIFI_CONNECTED) {
                if (info->ipAddress != 0) {
                    w->local_ip = info->ipAddress;
                    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork IP (%s)", __inet_ntoa(info->ipAddress, addrbuf));
                    if (WIFI_SUCCESS == GetLocalWifiGw((int*const)&w->gw)) {
                        LZ_HARDWARE_LOGD(LOG_TAG, "network GW (%s)", __inet_ntoa(w->gw, addrbuf));
                    }
                    if (WIFI_SUCCESS == GetLocalWifiNetmask((int*const)&w->netmask)) {
                        LZ_HARDWARE_LOGD(LOG_TAG, "network NETMASK (%s)", __inet_ntoa(w->netmask, addrbuf));
                    }
                    ret = 0;
                    goto connect_done;
                }
                else
                {
                    ret = 1;
                }
            }
            else
            {
                ret = 2;
            }
        }
        else
        {
            ret = 3;
        }

        do{
            m_wifi.wifi_queue_rev_fn(0);
            LOS_Msleep(1000);
        }          
        while(w->link_stop);
        // printf("@wifi link:%d\n", ret);
        LOS_Msleep(1000);
    }

connect_done:
    return ret;
}

int get_ap_info(void *wifi)
{
    struct in_addr addr;
    wifi_task_t *w = (wifi_task_t*)wifi;
    char addrbuf[ADDRBUF_LEN] = {0};
    w->remote_ip = 0;

    do
    {
        addr = DHCPD_FindAddress();
        LOS_Msleep(5000);
    }while(addr.s_addr == 0 || (unsigned char)((addr.s_addr>>24)&0xff) <= 150);

    if (WIFI_SUCCESS == GetLocalWifiIp((int*const)&w->local_ip)) {
        LZ_HARDWARE_LOGD(LOG_TAG, "network ip (%s)", __inet_ntoa(w->local_ip, addrbuf));
    }

    if (WIFI_SUCCESS == GetLocalWifiGw((int*const)&w->gw)) {
        LZ_HARDWARE_LOGD(LOG_TAG, "network GW (%s)", __inet_ntoa(w->gw, addrbuf));
    }
    if (WIFI_SUCCESS == GetLocalWifiNetmask((int*const)&w->netmask)) {
        LZ_HARDWARE_LOGD(LOG_TAG, "network NETMASK (%s)", __inet_ntoa(w->netmask, addrbuf));
    }
    w->remote_ip = (addr.s_addr&0x00ffffff)|(uint32_t)(150<<24);

    printf("连接设备IP:%s\n", __inet_ntoa(w->remote_ip, addrbuf));
    LOS_Msleep(5000);

    return 0;
}

int net_set_remote_ip(char *ip, uint16_t port)
{
    wifi_task_t *w = &m_wifi;

    w->remote_ip   = inet_addr(ip);
    w->remote_port = port;

    SetWifiModeOff();  
    SetApModeOff();

    return 0;
}

/***************************************************************
* 函数名称: wifi_link_test_swtimer_callback
* 说    明: wifi 连接检测
* 参    数: uint32_t args
* 返 回 值: 无
***************************************************************/
void wifi_link_test_swtimer_callback(uint32_t args)
{
    int ret = 1;
    wifi_task_t  *w = (wifi_task_t*)args;
    static  int retry = 0;

    if(w->link_state.wifi == E_DATA_WIFI_NOLINK)
    {
        m_wifi.wifi_queue_rev_fn(1);
        if(w->link_stop) return;

        if(++retry > 4)
        {
            if(w->mode == WIFI_MODE_AP)
            {
                if(w->ap->get_link_state_fn) ret = w->ap->get_link_state_fn(w);
            }
            else
            {
                if(w->sta->get_link_state_fn) ret = w->sta->get_link_state_fn(w);
            }

            if(ret != 0)
            {
                printf("\n@wifi no link %d\n\n", ret);
            }
            else
            {
                printf("\n@wifi link %s\n\n", ret==0?"success":"err");
                return;
            }
            retry = 0;
            if(!w->relink_flag) w->relink_flag = true;
        }
    }
    else
    {
        if(w->mode == WIFI_MODE_AP)
        {
            if(w->ap->get_link_state_fn) ret = w->ap->get_link_state_fn(w);
        }
        else
        {
            if(w->sta->get_link_state_fn) ret = w->sta->get_link_state_fn(w);
        }

        if(ret != 0)
        {
            printf("\nwifi no link %d\n\n", ret);
            w->link_state.wifi = E_DATA_WIFI_NOLINK;
            if(w->client_pc->thread_start_flag == UDP_TASK_STATE_RUN)  w->client_pc->thread_start_flag  = UDP_TASK_STATE_STOP;
            if(w->client_ph->thread_start_flag == UDP_TASK_STATE_RUN) w->client_ph->thread_start_flag = UDP_TASK_STATE_STOP;
            if(w->server->thread_start_flag == UDP_TASK_STATE_RUN)  w->server->thread_start_flag  = UDP_TASK_STATE_STOP;
            if(w->client_pc->rmthread_start_flag == UDP_TASK_STATE_RUN)  w->client_pc->rmthread_start_flag  = UDP_TASK_STATE_STOP;
            if(w->client_ph->rmthread_start_flag == UDP_TASK_STATE_RUN) w->client_ph->rmthread_start_flag = UDP_TASK_STATE_STOP;
            if(w->server->rmthread_start_flag == UDP_TASK_STATE_RUN)  w->server->rmthread_start_flag  = UDP_TASK_STATE_STOP;
            // if(w->swtmr.stop_fn) w->swtmr.stop_fn(&w->swtmr, STR_WIFI_SWTMR_LINK_TEST);
        }
        else
        {
            // printf("\nwifi link %s\n\n", ret==0?"success":"err");
            w->link_state.wifi = E_DATA_WIFI_LINK;
        }
        if(w->msg_send_to_lcd_fn) w->msg_send_to_lcd_fn(w, w->link_state.wifi);
    }
}

//发消息到lcd
void wifi_msg_send_to_lcd(void *wifi, data_wifi_type_e msg)
{
    wifi_task_t *w = (wifi_task_t*)wifi;
    msg_cmd_t   *c = &w->lcd_msg.cmd;

    c->msg_type      = E_MSG_CMD;
    c->data_type     = E_DATA_WIFI_LCD;
    c->len           = get_msg_len(w->lcd_msg);
    c->wifi_lcd.type = msg;

    // printf_hex(printf, (uint8_t*)&w->lcd_msg, sizeof(app_msg_t));
    // printf("crc main:%#X\n", w->lcd_msg.cmd.crc);

    if (0 != LOS_QueueWriteCopy(g_lcd_queue, &w->lcd_msg, sizeof(app_msg_t), QUEUE_DEFAULT_WRITE_TIME_MS))
    {
        WRITE_QUEUE_ERR_LOG(g_lcd_queue);
    }               
}

//发消息到设备
void msg_send_to_dev(void *msg)
{
    dev_msg_t *m = (dev_msg_t*)msg;

    if (0 != LOS_QueueWriteCopy(g_netrev_queue, m, sizeof(dev_msg_t), QUEUE_DEFAULT_WRITE_TIME_MS))
    {
        WRITE_QUEUE_ERR_LOG(g_netrev_queue);
    }  
}


//--------------------------------------------------------------------------------------
// udp层api
//--------------------------------------------------------------------------------------
//udp客户端接收消息处理 和超时检测
void udp_client_pc_read_process(void *wifi)
{
    TASK_PROCESS_START_LOG();
    wifi_task_t *w = (wifi_task_t*)wifi;
    char buf[BUFF_LEN], addrbuf[ADDRBUF_LEN];
    int count = 0;
    socklen_t len;
    struct sockaddr_in client_addr = {0}, last_client_addr = {0};

    w->client_pc->rmthread_start_flag = UDP_TASK_STATE_RUN;

    while (1)
    {   
        memset(buf, 0, BUFF_LEN);
        count = recvfrom(w->client_pc->socket_fd, buf, BUFF_LEN, 0, (struct sockaddr*)&client_addr, &len);       //recvfrom是阻塞函数，没有数据就一直阻塞
        if(count == -1)
        {
            printf(LOG_TAG_CLIENT"recieve data fail!\n", __LINE__);
            w->client_pc->time_out_flag = 1;
            break;
        }
        else
        {
            w->client_pc->time_out_flag = 0;
            app_msg_data_t *m = (app_msg_data_t *)buf;

            if(memcmp(&last_client_addr, &client_addr, sizeof(struct sockaddr_in)) != 0)
            {
                last_client_addr = client_addr;
                printf(LOG_TAG_CLIENT"local  addr:%s port:%d\n", __LINE__, __inet_ntoa(w->local_ip, addrbuf), w->local_port);
                printf(LOG_TAG_CLIENT"remote addr:%s port:%u\n", __LINE__, __inet_ntoa(client_addr.sin_addr, addrbuf), ntohs(client_addr.sin_port));
            }
            printf(LOG_TAG_CLIENT"len:%d, rev:%s\n\n", __LINE__, count, buf);
            FifoPut(w->read->fifo, m);
        }

        if(w->client_pc->rmthread_start_flag != UDP_TASK_STATE_RUN)
        {
            w->client_pc->time_out_flag = 1;
            goto end;
        } 
        LOS_Msleep(100);
    }

end:
    printf(LOG_TAG_CLIENT"read msg task end\n", __LINE__);
    w->client_pc->rmthread_start_flag = UDP_TASK_STATE_END;
    while(1)
    {
        LOS_Msleep(100);
    }
}

//udp服务端接收消息处理 和超时检测
void udp_server_read_process(void *wifi)
{
    TASK_PROCESS_START_LOG();
    wifi_task_t *w = (wifi_task_t*)wifi;
    char buf[BUFF_LEN], addrbuf[ADDRBUF_LEN];
    int count = 0;
    socklen_t len;
    struct sockaddr_in client_addr = {0}, last_client_addr = {0};

    w->server->rmthread_start_flag = UDP_TASK_STATE_RUN;

    while (1)
    {   
        memset(buf, 0, BUFF_LEN);
        count = recvfrom(w->server->socket_fd, buf, BUFF_LEN, 0, (struct sockaddr*)&client_addr, &len);       //recvfrom是阻塞函数，没有数据就一直阻塞
        if(count == -1)
        {
            printf(LOG_TAG_SERVER"recieve data fail!\n", __LINE__);
            w->server->time_out_flag = 1;
            break;
        }
        else
        {
            w->link_state.server = E_DATA_WIFI_SERVER_LINK;
            if(w->msg_send_to_lcd_fn) w->msg_send_to_lcd_fn(w, w->link_state.server);

            w->server->sockaddr = client_addr;
            w->server->time_out_flag = 0;
            app_msg_data_t *rev;
            rev = (app_msg_data_t*)buf;
            int msg_len = get_dev_req_msg_len(rev->dev_msg);
            msg_len = msg_len == 0 ? count:msg_len;
            msg_len = msg_len > count ? count: msg_len;
            if(memcmp(&last_client_addr, &client_addr, sizeof(struct sockaddr_in)) != 0)
            {
                last_client_addr = client_addr;
                printf(LOG_TAG_SERVER"local  addr:%s port:%d\n", __LINE__, __inet_ntoa(w->local_ip, addrbuf), w->local_port);
                printf(LOG_TAG_SERVER"remote addr:%s port:%u len:%d\n", __LINE__, __inet_ntoa(client_addr.sin_addr, addrbuf), ntohs(client_addr.sin_port), len);
            }
            printf(LOG_TAG_SERVER"socket:%d,cmd:%d,len:%d,%d\n\n", __LINE__, w->server->socket_fd, Swap16(rev->dev_msg.type_head.cmd_type), count, msg_len);
            // dump_hex(printf, __FUNCTION__, buf, msg_len<32?msg_len:32);
            FifoPut(w->read->fifo, rev);
        }

        if(w->server->rmthread_start_flag != UDP_TASK_STATE_RUN)
        {
            w->server->time_out_flag = 1;
            goto end;
        } 
        LOS_Msleep(100);
    }

end:
    printf(LOG_TAG_SERVER"read msg task end\n", __LINE__);
    w->server->rmthread_start_flag = UDP_TASK_STATE_END;
    while(1)
    {
        LOS_Msleep(5000);
    }
}


//ucp客户端消息处理
void udp_client_pc_write_msg(void* args)
{
    wifi_task_t *w = (wifi_task_t*)args;
    int ret, len, count = 0;
    char addrbuf[ADDRBUF_LEN] = {0};
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    printf(LOG_TAG_CLIENT"remote addr:%s port:%u\n\n", __LINE__, __inet_ntoa(((struct sockaddr_in*)&w->client_pc->sockaddr)->sin_addr, addrbuf), ntohs(((struct sockaddr_in*)&w->client_pc->sockaddr)->sin_port));
	connect(w->client_pc->socket_fd, (const struct sockaddr *)&w->client_pc->sockaddr, client_addr_len);
    getsockname(w->client_pc->socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);

    w->link_state.client_pc = E_DATA_WIFI_CLIENT_PC_LINK;

    // create_task(&w->client_pc->rmthread_id,       20480, 24,   udp_client_pc_read_process,                w, "udp_client_pc_read_process");
    int display = 0;

    while (1)
    {
        app_msg_data_t m = {0};

        if(w->client_pc->thread_start_flag != UDP_TASK_STATE_RUN) break;

        count = FifoGet(w->write_pc->fifo, &m);

        if(count)
        {//有数据需要发送
            len = get_dev_res_msg_len(m.dev_msg);
            if(len == 0) 
            {
                printf(LOG_TAG_CLIENT"msg type err, msg len is 0!!!!!!\n", __LINE__);
                continue;
            }
            ret = sendto(w->client_pc->socket_fd, &m, len, 0, (struct sockaddr*)&w->client_pc->sockaddr, client_addr_len);        //发送信息给client
            if(!(Swap16(m.dev_msg.type_head.type) == E_DEV_SMART_HOUSE && Swap16(m.dev_msg.type_head.cmd_type) == E_SMART_HOUSE_CMD_REPORT_STATE))
            {
                if(display == 0)
                {
                    display = 1;
                    printf(LOG_TAG_CLIENT"local  addr:%s port:%u\n", __LINE__, __inet_ntoa(client_addr.sin_addr, addrbuf), ntohs(client_addr.sin_port));
                    printf(LOG_TAG_CLIENT"remote addr:%s port:%u\n", __LINE__, __inet_ntoa(((struct sockaddr_in*)&w->client_pc->sockaddr)->sin_addr, addrbuf), 
                                                                                     ntohs(((struct sockaddr_in*)&w->client_pc->sockaddr)->sin_port));
                }
                // printf(LOG_TAG_CLIENT"sendto pc,socket:%d,cmd:%d, %d,%d, ret:%d\n\n", __LINE__, w->client_pc->socket_fd, Swap16(m.dev_msg.type_head.cmd_type), count, len, ret);
                // dump_hex(printf, __FUNCTION__, &m, len<32?len:32);
            }
        }

        if(w->client_pc->time_out_flag)
        {//超时重连
            printf(LOG_TAG_CLIENT"timeout !\n", __LINE__);
            break;
        }
        LOS_Msleep(100);
    }

    w->link_state.client_pc = E_DATA_WIFI_CLIENT_PC_NOLINK;
    if(w->msg_send_to_lcd_fn) w->msg_send_to_lcd_fn(w, w->link_state.client_pc);

    int retry = 20;
    while(retry--)
    {
        if(w->client_pc->rmthread_start_flag == UDP_TASK_STATE_END) break;
        LOS_Msleep(500);
    }
    LOS_TaskDelete(w->client_pc->rmthread_id);
    w->client_pc->rmthread_start_flag = UDP_TASK_STATE_END;
    w->client_pc->time_out_flag = 0;
    lwip_close(w->client_pc->socket_fd);
}

//udp服务端消息处理
void udp_server_write_msg(void *args)
{
    wifi_task_t *w = (wifi_task_t*)args;
#if ((RESPONE_SEND_TYPE == RESPONE_SEND_TYPE_SINGLE_IP) || (RESPONE_SEND_TYPE == RESPONE_SEND_TYPE_SINGLE_IP_PORT))
    int display = 0;
#endif   

    while (1)
    {
        if(w->server->thread_start_flag != UDP_TASK_STATE_RUN) break;

#if ((RESPONE_SEND_TYPE == RESPONE_SEND_TYPE_SINGLE_IP) || (RESPONE_SEND_TYPE == RESPONE_SEND_TYPE_SINGLE_IP_PORT))
        char addrbuf[ADDRBUF_LEN] = {0};
        struct in_addr addr = {0};
        socklen_t client_addr_len = sizeof(struct sockaddr_in);
        __inet_ntoa(((struct sockaddr_in*)&w->server->sockaddr)->sin_addr, addrbuf);
        if(inet_aton(addrbuf, &addr) && strcmp(addrbuf, "0.0.0.0") != 0)
        {
            uint32_t len = 0, count;
            app_msg_data_t m = {0};
            count = FifoGet(w->write_pc->fifo, &m);
            if(count)
            {   // 有数据需要发送
#if (RESPONE_SEND_TYPE == RESPONE_SEND_TYPE_SINGLE_IP)                
                // 根据接收到扫描设备数据的ip往固定远端端口发送
                w->server->sockaddr.sin_port = htons(w->remote_port);
#endif
                len = get_dev_res_msg_len(m.dev_msg);
                if(len == 0) 
                {
                    printf(LOG_TAG_SERVER"msg type err, msg len is 0 !!!!!!\n", __LINE__);
                    continue;
                }                
                int ret = sendto(w->server->socket_fd, &m, len, 0, (const struct sockaddr*)&w->server->sockaddr, client_addr_len);        //发送信息给client
                if(Swap16(m.dev_msg.type_head.cmd_type) != E_SMART_HOUSE_CMD_REPORT_STATE)
                {
                    if(display == 0)
                    {
                        display = 1;
                        printf(LOG_TAG_SERVER"local  addr:%s port:%u\n", __LINE__, __inet_ntoa(w->local_ip, addrbuf), w->local_port);
                        printf(LOG_TAG_SERVER"remote addr:%s port:%u\n", __LINE__, __inet_ntoa(((struct sockaddr_in*)&w->server->sockaddr)->sin_addr, addrbuf), 
                                                                    ntohs(((struct sockaddr_in*)&w->server->sockaddr)->sin_port));
                    }
                    printf(LOG_TAG_SERVER"sendto pc,socket:%d,cmd:%d, %d,%d, ret:%d\n\n", __LINE__, 
                                                                w->server->socket_fd, Swap16(m.dev_msg.type_head.cmd_type), count, len, ret);
                }
            }
        }
#endif
        if(w->server->time_out_flag)
        {//超时重连
            printf(LOG_TAG_SERVER"timeout !\n", __LINE__);
            break;
        }
        LOS_Msleep(100);
    }

    w->link_state.server = E_DATA_WIFI_SERVER_NOLINK;
    if(w->msg_send_to_lcd_fn) w->msg_send_to_lcd_fn(w, w->link_state.server);

    int retry = 20;
    while(retry--)
    {
        if(w->server->rmthread_start_flag == UDP_TASK_STATE_END) break;
        LOS_Msleep(500);
    }

    LOS_TaskDelete(w->server->rmthread_id);
    w->server->rmthread_start_flag = UDP_TASK_STATE_END;
    w->server->time_out_flag = 0;
    lwip_close(w->server->client_fd);
}

//--------------------------------------------------------------------------------------
// wifi层api
//--------------------------------------------------------------------------------------
//客户端处理
void wifi_client_pc_process(void* args)
{
    TASK_PROCESS_START_LOG();
    int ret;
    char addrbuf[ADDRBUF_LEN] = {0};
    wifi_task_t *w = (wifi_task_t*)args;
    w->client_pc->thread_start_flag = UDP_TASK_STATE_RUN;

    while(1)
    {
        if(w->client_pc->thread_start_flag != UDP_TASK_STATE_RUN) goto end;

        if(w->mode == WIFI_MODE_AP)
        {
            get_ap_info(w);
        }
        else
        {
            char ip_buf[ADDRBUF_LEN] = {0};
            uint32_t n = strrchr(__inet_ntoa(w->local_ip, addrbuf), '.') - addrbuf + 1;

            n = n > ADDRBUF_LEN ? ADDRBUF_LEN:n;
            strncpy(ip_buf, addrbuf, n);
            strcat(ip_buf, "255");
            // w->remote_ip = inet_addr(SERVER_IP);
            w->remote_ip = inet_addr(ip_buf);
        }

        w->client_pc->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);//AF_INET:IPV4;SOCK_STREAM:TCP
        if (w->client_pc->socket_fd < 0)
        {
            printf(LOG_TAG_CLIENT"create socket fail!\n", __LINE__);
            LOS_Msleep(w->client_pc->timeout.tv_sec*1000);
            continue;
        }

        /*设置调用close(socket)后,仍可继续重用该socket。调用close(socket)一般不会立即关闭socket，而经历TIME_WAIT的过程。*/
        //third_party/musl/porting/linux/user/include/sys/socket.h
        int flag = 1;
        ret = setsockopt(w->client_pc->socket_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
        if (ret != 0) {
            printf(LOG_TAG_CLIENT"setsockopt fail, ret[%d]-%d!\n", __LINE__, ret);
        }

        ret = setsockopt(w->client_pc->socket_fd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag));
        if (ret != 0) {
            printf(LOG_TAG_CLIENT"setsockopt fail, ret[%d]-%d!\n", __LINE__, ret);
        }

        ret = setsockopt(w->client_pc->socket_fd, SOL_SOCKET, SO_SNDTIMEO,(const char*)&w->client_pc->timeout, sizeof(w->client_pc->timeout));
        if (ret != 0) {
            printf(LOG_TAG_CLIENT"setsockopt fail, ret[%d]-%d!\n", __LINE__, ret);
        }
        ret = setsockopt(w->client_pc->socket_fd, SOL_SOCKET, SO_RCVTIMEO,(const char*)&w->client_pc->timeout, sizeof(w->client_pc->timeout));
        if (ret != 0) {
            printf(LOG_TAG_CLIENT"setsockopt fail, ret[%d]!\n", __LINE__, ret);
        }

        struct sockaddr_in local_addr;
        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = w->local_ip;
        local_addr.sin_port = htons(w->remote_port);
        //绑定本地ip端口号
        ret = bind(w->client_pc->socket_fd, (struct sockaddr*)&local_addr, sizeof(local_addr));

        memset(&w->client_pc->sockaddr, 0, sizeof(w->client_pc->sockaddr));
        w->client_pc->sockaddr.sin_family = AF_INET;
        // w->client_pc->sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);       
        w->client_pc->sockaddr.sin_addr.s_addr = w->remote_ip; //*.*.*.255
        w->client_pc->sockaddr.sin_port = htons(w->remote_port);
        printf("\n"LOG_TAG_CLIENT"remote addr:%d<%s:%d>\n", __LINE__, w->client_pc->socket_fd, __inet_ntoa(w->client_pc->sockaddr.sin_addr, addrbuf), ntohs(w->client_pc->sockaddr.sin_port));
        if(w->client_pc->tcp_write_msg_fn) w->client_pc->tcp_write_msg_fn(w);

        LOS_Msleep(1000);
    }

end:
    printf(LOG_TAG_CLIENT"write_pc msg task end\n", __LINE__);
    w->client_pc->thread_start_flag = UDP_TASK_STATE_END;
    while (1)
    {
        LOS_Msleep(1000);
    }
}

//服务端处理
void wifi_server_process(void* args)
{
    TASK_PROCESS_START_LOG();
    int ret;
    char addrbuf[ADDRBUF_LEN] = {0};
    wifi_task_t *w = (wifi_task_t*)args;
    w->server->thread_start_flag = UDP_TASK_STATE_RUN;

    while(1)
    {
        if(w->server->thread_start_flag != UDP_TASK_STATE_RUN) goto end;

        if(w->mode == WIFI_MODE_AP)
        {
            get_ap_info(w);
        }
        else
        {
            char ip_buf[ADDRBUF_LEN] = {0};
            uint32_t n = strrchr(__inet_ntoa(w->local_ip, addrbuf), '.') - addrbuf + 1;

            n = n > ADDRBUF_LEN ? ADDRBUF_LEN:n;
            strncpy(ip_buf, addrbuf, n);
            strcat(ip_buf, "255");
            // printf("local ip:%s,{%s}(%d)%s\n", addrbuf, strrchr(addrbuf, '.'), n, ip_buf);
            // w->remote_ip = inet_addr(SERVER_IP);
            w->remote_ip = inet_addr(ip_buf);
        }

        w->server->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);         //AF_INET:IPV4;SOCK_STREAM:TCP,SOCK_DGRAM:UDP
        if (w->server->socket_fd < 0)
        {
            printf(LOG_TAG_SERVER"create socket fail!\n", __LINE__);
            LOS_Msleep(w->server->timeout.tv_sec*1000);
            continue;
        }

        /*设置调用close(socket)后,仍可继续重用该socket。调用close(socket)一般不会立即关闭socket，而经历TIME_WAIT的过程。*/
        int flag = 1;
        ret = setsockopt(w->server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
        if (ret != 0) {
            printf(LOG_TAG_SERVER"setsockopt fail, ret[%d]!\n", __LINE__, ret);
        }

        ret = setsockopt(w->server->socket_fd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag));
        if (ret != 0) {
            printf(LOG_TAG_CLIENT"setsockopt fail, ret[%d]!\n", __LINE__, ret);
        }

        // ret=setsockopt(w->server->socket_fd, SOL_SOCKET, SO_SNDTIMEO,(const char*)&w->server->timeout, sizeof(w->server->timeout));
        // if (ret != 0) {
        //     printf(LOG_TAG_SERVER"setsockopt fail, ret[%d]!\n", __LINE__, ret);
        // }
        // ret=setsockopt(w->server->socket_fd, SOL_SOCKET, SO_RCVTIMEO,(const char*)&w->server->timeout, sizeof(w->server->timeout));
        // if (ret != 0) {
        //     printf(LOG_TAG_SERVER"setsockopt fail, ret[%d]!\n", __LINE__, ret);
        // }

        struct sockaddr_in serv_addr = {0};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
        // serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP); //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
        serv_addr.sin_port = htons(w->local_port);       //端口号，需要网络序转换
        /* 绑定服务器地址结构 */
        ret = bind(w->server->socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        // ret = lwip_bind(w->server->socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        if (ret < 0)
        {
            printf(LOG_TAG_SERVER"socket bind fail!\n", __LINE__);
            goto err;
        }
        printf("\n"LOG_TAG_SERVER"local:%d<%s:%d>\n", __LINE__, w->server->socket_fd, __inet_ntoa(serv_addr.sin_addr, addrbuf), ntohs(serv_addr.sin_port));
        
        create_task(&w->server->rmthread_id,       20480, 24,   udp_server_read_process,                w, "udp_server_read_process");

        if(w->server->tcp_write_msg_fn) w->server->tcp_write_msg_fn(w); //处理udp数据
err:        
        lwip_close(w->server->socket_fd);
        LOS_Msleep(3000);
    }

end:
    printf(LOG_TAG_SERVER"write msg task end\n", __LINE__);
    w->server->thread_start_flag = UDP_TASK_STATE_END;
    while (1)
    {
        LOS_Msleep(1000);
    }
}

// wifi 接收udp消息处理
void wifi_rmsg_process(void* args)
{
    TASK_PROCESS_START_LOG();
    int               ret = 0;
    app_msg_data_t    m   = {0};
    wifi_task_t      *w   = (wifi_task_t*)args;

    while(1)
    {
        memset(&m, 0, sizeof(app_msg_data_t));
        ret = FifoGet(w->read->fifo, &m);
        if(ret)
        {
            // uint8_t *devnum = m.dev_msg.head.num;
            // printf(LOG_TAG_INFO"wifi msg from udp!!!!!!!!\n", __LINE__);
            // printf(LOG_TAG_INFO"devnum:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,dev_type:%04x cmd_type:%04x, response:%d\n", __LINE__,
            //     devnum[0], devnum[1], devnum[2], devnum[3], devnum[4],  devnum[5],
            //     devnum[6], devnum[7], devnum[8], devnum[9], devnum[10], devnum[11],
            //     Swap16(m.dev_msg.head.type), Swap16(m.dev_msg.type_head.cmd_type), m.dev_msg.type_head.response);
            m.dev_msg.head.type          = Swap16(m.dev_msg.head.type);
            m.dev_msg.type_head.cmd_type = Swap16(m.dev_msg.type_head.cmd_type);
            msg_send_to_dev(&m.dev_msg);
        }
        LOS_Msleep(200);
    }
    w->read->thread_id = 0;
}

// wifi 发送udp消息处理
void wifi_wmsg_process(void* args)
{
    TASK_PROCESS_START_LOG();
    wifi_task_t    *w = (wifi_task_t*)args;
    app_msg_data_t  m = {0};
    uint32_t      len = sizeof(app_msg_data_t);

    while(1)
    {
        memset(&m, 0, sizeof(app_msg_data_t));
        len = sizeof(app_msg_data_t);
        if(LOS_QueueReadCopy(g_tcp_queue, &m, &len, LOS_WAIT_FOREVER) == LOS_OK)
        {    
            // printf(LOG_TAG_INFO"wif msg to udp cmd:%x,len:%d\n", __LINE__, Swap16(m.dev_msg.type_head.cmd_type), get_dev_res_msg_len(m.dev_msg));
            // uint8_t *devnum = m.dev_msg.head.num;
            // printf(LOG_TAG_INFO"put devnum:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,dev_type:%04x cmd_type:%04x, response:%d\n", __LINE__,
            //     devnum[0], devnum[1], devnum[2], devnum[3], devnum[4],  devnum[5],
            //     devnum[6], devnum[7], devnum[8], devnum[9], devnum[10], devnum[11],
            //     Swap16(m.dev_msg.head.type), Swap16(m.dev_msg.type_head.cmd_type), m.dev_msg.type_head.response);
            FifoPut(w->write_pc->fifo, &m);
        }
    }
    w->write_pc->thread_id = 0;
}

//wifi 重连处理
void wifi_relink_process(void* args)
{
    TASK_PROCESS_START_LOG();
    wifi_task_t *w = (wifi_task_t*)args;

    while(1)
    {
        if(w->relink_flag)
        {
            SetWifiModeOff();  
            SetApModeOff();

            if(w->mode == WIFI_MODE_AP)
            {
                SetApModeOn();       //ap模式
                printf("wifi AP mode retry\n");
            }
            else
            {
                SetWifiModeOn();     //sta模式
                printf("wifi STA mode retry\n");
            }
            w->relink_flag = false;
        }
        LOS_Msleep(1000);
    }
}

//--------------------------------------------------------------------------------------
// wifi创建任务
//--------------------------------------------------------------------------------------
//udp 任务启动包括WiFi模式启动
void udp_task_start(void *wifi)
{
    wifi_task_t *w = (wifi_task_t*)wifi;
    SetWifiModeOff();  
    SetApModeOff();

    if(w->swtmr.start_fn) w->swtmr.start_fn(&w->swtmr, STR_WIFI_SWTMR_LINK_TEST);

    if(w->mode == WIFI_MODE_AP)
    {
        SetApModeOn();       //ap模式
        printf(LOG_TAG_INFO"wifi AP mode\n", __LINE__);
        get_ap_info(w);
        if(w->ap->get_link_state_fn) w->link_state.wifi = w->ap->get_link_state_fn(w);
    }
    else
    {
        SetWifiModeOn();     //sta模式
        printf(LOG_TAG_INFO"wifi STA mode\n", __LINE__);
        get_sta_info(w);
        if(w->sta->get_link_state_fn) w->link_state.wifi = w->sta->get_link_state_fn(w);
    }

    if(w->msg_send_to_lcd_fn) w->msg_send_to_lcd_fn(w, w->link_state.wifi);
#if (RESPONE_SEND_TYPE == RESPONE_SEND_TYPE_BROADCAST)
    printf(LOG_TAG_INFO" Broadcast response \n", __LINE__);
    if(w->client_pc->thread_id) LOS_TaskDelete(w->client_pc->thread_id);
    create_task(&w->client_pc->thread_id,    10240, 24,   wifi_client_pc_process,                w, "wifi_client_pc_process");
#elif (RESPONE_SEND_TYPE == RESPONE_SEND_TYPE_SINGLE_IP)
    printf(LOG_TAG_INFO" Single IP port:%d response \n", __LINE__, w->remote_port);
#elif (RESPONE_SEND_TYPE == RESPONE_SEND_TYPE_SINGLE_IP_PORT)
    printf(LOG_TAG_INFO" Single IP response \n", __LINE__);
#endif

    if(w->server->thread_id) LOS_TaskDelete(w->server->thread_id);
    create_task(&w->server->thread_id,       10240, 24,   wifi_server_process,                   w, "wifi_server_process");
}

//接收队列消息用于重连判断
void wifi_queue_rev(uint32_t to)
{
    unsigned int ret = LOS_OK;
    app_msg_t    m   = {0};
    uint32_t     len = sizeof(app_msg_t);

    ret = LOS_QueueReadCopy(g_wifi_queue, &m, &len, to);
    do
    {
        if(ret != LOS_OK) break;
        // if(calc_msg_crc16(m)) break;
        if(m.type != E_MSG_CMD) break;
        if(m.cmd.data_type != E_DATA_CTRL_WIFI) break;
        m_wifi.link_stop = m.cmd.ctrl_wifi.cmd == 0? true:false;
        printf("wifi re stop:%d\n", m_wifi.link_stop);
    }while(0);           
}

//WiFi处理 建立udp客户端，服务器
void wifi_process(void *args)
{
    TASK_PROCESS_START_LOG();
    // printf("wifi:%u\n", sizeof(wifi_task_t));

    FifoInit(m_wifi.read->fifo);
    FifoInit(m_wifi.write_pc->fifo);
    
    wifi_task_t *w = &m_wifi;
    w->remote_ip   = inet_addr(SERVER_IP);

    create_task(&w->relthread_id,          4096, 24,   wifi_relink_process,              w, "wifi_relink_process");

    m_wifi.swtmr.args = &m_wifi;
    m_wifi.swtmr.init_fn(WIFI_SWTMR_LINK_TEST_PERIOD_MS, wifi_link_test_swtimer_callback, STR_WIFI_SWTMR_LINK_TEST, (uint32_t *)&m_wifi.swtmr);//连接状态  
    if(m_wifi.udp_task_start_fn) m_wifi.udp_task_start_fn(&m_wifi);

    create_task(&w->read->thread_id,       4096, 24,   wifi_rmsg_process,                w, "wifi_rmsg_process");
    create_task(&w->write_pc->thread_id,   4096, 24,   wifi_wmsg_process,                w, "wifi_wmsg_process");

    LOS_Msleep(5000);
    while(1)
    {
        // printf("wifi task state:%d-%d %d-%d %d-%d, wifilink:%d\n", m_wifi.client_pc->thread_start_flag, m_wifi.client_pc->rmthread_start_flag,
        // m_wifi.client_ph->thread_start_flag, m_wifi.client_ph->rmthread_start_flag, 
        // m_wifi.server->thread_start_flag, m_wifi.server->rmthread_start_flag,
        // wifi_get_connect_status_internal());

        if(((m_wifi.client_pc->thread_start_flag == UDP_TASK_STATE_END && (m_wifi.client_pc->rmthread_start_flag == UDP_TASK_STATE_END || m_wifi.client_pc->rmthread_start_flag == UDP_TASK_STATE_INIT)) 
                || (m_wifi.client_pc->thread_start_flag == UDP_TASK_STATE_INIT && m_wifi.client_pc->rmthread_start_flag == UDP_TASK_STATE_INIT))
        && ((m_wifi.server->thread_start_flag == UDP_TASK_STATE_END && (m_wifi.server->rmthread_start_flag == UDP_TASK_STATE_END || m_wifi.server->rmthread_start_flag == UDP_TASK_STATE_INIT)) 
                || (m_wifi.server->thread_start_flag == UDP_TASK_STATE_INIT && m_wifi.server->rmthread_start_flag == UDP_TASK_STATE_INIT)))
        {
            printf("wifi udp task retry\n");
            if(m_wifi.client_pc->thread_start_flag == UDP_TASK_STATE_END)
            {
                LOS_TaskDelete(m_wifi.client_pc->thread_id);
                m_wifi.client_pc->thread_id     = 0;
            } 
            m_wifi.client_pc->thread_start_flag = UDP_TASK_STATE_INIT;

            if(m_wifi.client_pc->rmthread_start_flag == UDP_TASK_STATE_END)
            {
                LOS_TaskDelete(m_wifi.client_pc->rmthread_id);
                m_wifi.client_pc->rmthread_id     = 0;
            } 
            m_wifi.client_pc->rmthread_start_flag = UDP_TASK_STATE_INIT;

            if(m_wifi.server->thread_start_flag == UDP_TASK_STATE_END)
            {
                LOS_TaskDelete(m_wifi.server->thread_id);
                m_wifi.server->thread_id     = 0;
            } 
            m_wifi.server->thread_start_flag = UDP_TASK_STATE_INIT;

            if(m_wifi.server->rmthread_start_flag == UDP_TASK_STATE_END)
            {
                LOS_TaskDelete(m_wifi.server->rmthread_id);
                m_wifi.server->rmthread_id     = 0;
            } 
            m_wifi.server->rmthread_start_flag = UDP_TASK_STATE_INIT;

            if(m_wifi.udp_task_start_fn) m_wifi.udp_task_start_fn(&m_wifi);
        }

        m_wifi.wifi_queue_rev_fn(2000);
    }
}

