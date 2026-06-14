#include "com_msg.h"
#include "log.h"

//获取消息长度
uint16_t get_msg_len(app_msg_t m)
{
    switch(m.type)
    {  
        case E_MSG_CMD:     
            return sizeof(msg_cmd_t);
        case E_MSG_E53_C1:  
            return sizeof(msg_e53_c1_t);
        case E_MSG_E53_C2:  
            return sizeof(msg_e53_c2_t);
        case E_MSG_E53_C3: 
            return sizeof(msg_e53_c3_t);
        case E_MSG_E53_C4:  
            return sizeof(msg_e53_c4_t);
        case E_MSG_E53_C5:  
            return sizeof(msg_e53_c5_t);
        case E_MSG_E53_C6:  
            return sizeof(msg_e53_c6_t);
        case E_MSG_E53_C7:  
            return sizeof(msg_e53_c7_t);
        case E_MSG_E53_C8:  
            return sizeof(msg_e53_c8_t);
        case E_MSG_E53_C9:  
            return sizeof(msg_e53_c9_t);
        case E_MSG_E53_C10:
            return sizeof(msg_e53_c10_t);
        case E_MSG_E53_C11:
            return sizeof(msg_e53_c11_t);
        case E_MSG_E53_C12:
            return sizeof(msg_e53_c12_t);
        default:     
            return 0;
    }
}



//计算消息crc 正确返回0 
int calc_msg_crc16(app_msg_t msg)
{
    uint16_t crc = 0;

    switch(msg.type)
    {
        case E_MSG_CMD:
        case E_MSG_MODE:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_cmd_t)-2);
            if(msg.cmd.crc != crc)
            {
                printf("msg crc err:%#x-%#x\n", msg.cmd.crc, crc);
                return msg.type+1;
            }
            break;
        case E_MSG_E53_C1:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c1_t)-2);
            if(msg.e53_c1.crc != crc)
            {
                printf("%s c1 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c1.crc, crc);
                return msg.type+1;
            }
            break;
        case E_MSG_E53_C2:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c2_t)-2);
            if(msg.e53_c2.crc != crc)
            {
                printf("%s c2 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c2.crc, crc);
                return msg.type+1;
            }
            break;
        case E_MSG_E53_C3:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c3_t)-2);
            if(msg.e53_c3.crc != crc)
            {
                printf("%s c3 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c3.crc, crc);
                return msg.type+1;
            }
            break;
        case E_MSG_E53_C4:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c4_t)-2);
            if(msg.e53_c4.crc != crc)
            {
                printf("%s c4 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c4.crc, crc);
                return msg.type+1;
            }
            break;
        case E_MSG_E53_C5:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c5_t)-2);
            if(msg.e53_c5.crc != crc)
            {
                printf("%s c5 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c5.crc, crc);
                return msg.type+1;
            }
            break;
        case E_MSG_E53_C6:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c5_t)-2);
            if(msg.e53_c6.crc != crc)
            {
                printf("%s c6 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c6.crc, crc);
                return msg.type+1;
            }
            break;
        case E_MSG_E53_C7:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c7_t)-2);
            if(msg.e53_c7.crc != crc)
            {
                printf("%s c7 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c7.crc, crc);
                return msg.type+1;
            }            
            break;
        case E_MSG_E53_C8:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c8_t)-2);
            if(msg.e53_c8.crc != crc)
            {
                printf("%s c8 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c8.crc, crc);
                return msg.type+1;
            }
            break;
        case E_MSG_E53_C9:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c9_t)-2);
            if(msg.e53_c9.crc != crc)
            {
                printf("%s c9 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c9.crc, crc);
                return msg.type+1;
            }
            break;
        case E_MSG_E53_C10:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c10_t)-2);
            if(msg.e53_c10.crc != crc)
            {
                printf("%s c10 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c10.crc, crc);
                return msg.type+1;
            }
            break;
        case E_MSG_E53_C11:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c11_t)-2);
            if(msg.e53_c11.crc != crc)
            {
                printf("%s c11 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c11.crc, crc);
                return msg.type+1;
            }
            break;
        case E_MSG_E53_C12:
            crc = calc_crc16((uint8_t*)&msg, sizeof(msg_e53_c12_t)-2);
            if(msg.e53_c12.crc != crc)
            {
                printf("%s c12 msg crc err:%#x-%#x\n", __FUNCTION__, msg.e53_c12.crc, crc);
                return msg.type+1;
            }
            break;
        default:
            return E_MSG_MAX;
            break;
    }
    return 0;
}

//设置crc
void set_msg_crc16(app_msg_t *msg)
{
    switch(msg->type)
    {  
        case E_MSG_CMD:     
            msg->cmd.crc = calc_crc16((uint8_t*)msg, sizeof(msg_cmd_t)-2);
            break;
        case E_MSG_E53_C1:  
            msg->e53_c1.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c1_t)-2);
            break;
        case E_MSG_E53_C2:
            msg->e53_c2.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c2_t)-2);
            break;
        case E_MSG_E53_C3: 
            msg->e53_c3.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c3_t)-2);
            break;
        case E_MSG_E53_C4:  
            msg->e53_c4.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c4_t)-2);
            break;
        case E_MSG_E53_C5:  
            msg->e53_c5.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c5_t)-2);
            break;
        case E_MSG_E53_C6:  
            msg->e53_c6.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c5_t)-2);
            break;
        case E_MSG_E53_C7:  
            msg->e53_c7.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c7_t)-2);
            break;
        case E_MSG_E53_C8:  
            msg->e53_c8.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c8_t)-2);
            break;
        case E_MSG_E53_C9:  
            msg->e53_c9.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c9_t)-2);
            break;
        case E_MSG_E53_C10:
            msg->e53_c10.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c10_t)-2);
            break;
        case E_MSG_E53_C11:
            msg->e53_c11.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c11_t)-2);
            break;
        case E_MSG_E53_C12:
            msg->e53_c12.crc = calc_crc16((uint8_t*)msg, sizeof(msg_e53_c12_t)-2);
            break;
        default:     
            break;
    }
}

// 获取小屋接收服务器消息长度
int get_smart_house_req_msg_len(smart_house_cmd_type_e type)
{
    switch(type)
    {
        case E_SMART_HOUSE_CMD_RESET:
        case E_SMART_HOUSE_CMD_SEARCH_DEV:
        case E_SMART_HOUSE_CMD_REPORT_STATE:
            return sizeof(msg_smart_house_head_t);
        case E_SMART_HOUSE_CMD_OPEN_DOOR:
        case E_SMART_HOUSE_CMD_OPEN_CAR_DOOR:
        case E_SMART_HOUSE_CMD_OPEN_WINDOWS:
        case E_SMART_HOUSE_CMD_OPEN_LAMP:
            return sizeof(msg_smart_house_on_off_t);
        case E_SMART_HOUSE_CMD_SET_SMOKE_ALRAM_VAL:
        case E_SMART_HOUSE_CMD_SET_BRIGHTNESS_VAL:
            return sizeof(msg_smart_house_smoke_alarm_val_t);
        default:
            return 0;
    }
    return 0;
}

// 获取小屋响应服务器消息长度
int get_smart_house_res_msg_len(smart_house_cmd_type_e type)
{
    switch(type)
    {
        case E_SMART_HOUSE_CMD_MAX:
        case E_SMART_HOUSE_CMD_RESET:
        case E_SMART_HOUSE_CMD_SEARCH_DEV:
        case E_SMART_HOUSE_CMD_SET_SMOKE_ALRAM_VAL:
        case E_SMART_HOUSE_CMD_SET_BRIGHTNESS_VAL:
            return sizeof(msg_smart_house_head_t);
        case E_SMART_HOUSE_CMD_REPORT_STATE:
            return sizeof(msg_smart_house_report_state_t);
        case E_SMART_HOUSE_CMD_OPEN_DOOR:
        case E_SMART_HOUSE_CMD_OPEN_CAR_DOOR:
        case E_SMART_HOUSE_CMD_OPEN_WINDOWS:
        case E_SMART_HOUSE_CMD_OPEN_LAMP:
            return sizeof(msg_smart_house_on_off_t);
        default:
            return 0;
    }
    return 0;
}

// 获取医疗接收服务器消息长度
int get_smart_medical_req_msg_len(smart_medical_cmd_type_e type)
{
    switch(type)
    {
        case E_SMART_MEDICAL_CMD_RESET:
        case E_SMART_MEDICAL_CMD_SEARCH_DEV:
        case E_SMART_MEDICAL_CMD_REPORT_HRD:
        case E_SMART_MEDICAL_CMD_REPORT_TM:
            return sizeof(msg_smart_medical_head_t);
        case E_SMART_MEDICAL_CMD_REPORT_ME:
            return sizeof(msg_smart_medical_head_t)+5;
        case E_SMART_MEDICAL_CMD_SET_TASK:
            return sizeof(msg_smart_medical_set_task_t);
        default:
            return 0;
    }
    return 0;
}

// 获取医疗响应服务器消息长度
int get_smart_medical_res_msg_len(smart_medical_cmd_type_e type)
{
    switch(type)
    {
        case E_SMART_MEDICAL_CMD_MAX:
        case E_SMART_MEDICAL_CMD_RESET:
        case E_SMART_MEDICAL_CMD_SEARCH_DEV:
            return sizeof(msg_smart_medical_head_t);
        case E_SMART_MEDICAL_CMD_REPORT_HRD:
            return sizeof(msg_smart_medical_report_hrd_t);
        case E_SMART_MEDICAL_CMD_REPORT_TM:
            return sizeof(msg_smart_medical_report_tm_t);
        case E_SMART_MEDICAL_CMD_REPORT_ME:
            return sizeof(msg_smart_medical_report_me_t);
        case E_SMART_MEDICAL_CMD_SET_TASK:
            return sizeof(msg_smart_medical_set_task_t);
        default:
            return 0;
    }
    return 0;
}




int get_dev_req_msg_len(dev_msg_t d)
{
    switch (Swap16(d.head.type))
    {
    case E_DEV_SMART_HOUSE:
        return get_smart_house_req_msg_len(Swap16(d.smart_house.head.cmd_type));
    case E_DEV_SMART_MEDICAL:
        return get_smart_medical_req_msg_len(Swap16(d.smart_medical.head.cmd_type));    
    default:
        return 0;
    }
    return 0;
}

int get_dev_res_msg_len(dev_msg_t d)
{
    switch (Swap16(d.head.type))
    {
    case E_DEV_SMART_HOUSE:
        return get_smart_house_res_msg_len(Swap16(d.smart_house.head.cmd_type));
    case E_DEV_SMART_MEDICAL:
        return get_smart_medical_res_msg_len(Swap16(d.smart_medical.head.cmd_type));    
    default:
        return 0;
    }
    return 0;
}



int get_smart_house_data_len(smart_house_cmd_type_e type)
{
    switch(type)
    {
        case E_SMART_HOUSE_CMD_RESET:
        case E_SMART_HOUSE_CMD_SEARCH_DEV:
            return 0;
        case E_SMART_HOUSE_CMD_REPORT_STATE:
            return sizeof(msg1_smart_house_report_state_t)-sizeof(dev_type1_head_t);
        case E_SMART_HOUSE_CMD_OPEN_DOOR:
        case E_SMART_HOUSE_CMD_OPEN_CAR_DOOR:
        case E_SMART_HOUSE_CMD_OPEN_WINDOWS:
        case E_SMART_HOUSE_CMD_OPEN_LAMP:
            return sizeof(msg1_smart_house_on_off_t)-sizeof(dev_type1_head_t);
        case E_SMART_HOUSE_CMD_SET_SMOKE_ALRAM_VAL:
        case E_SMART_HOUSE_CMD_SET_BRIGHTNESS_VAL:
            return sizeof(msg1_smart_house_smoke_alarm_val_t)-sizeof(dev_type1_head_t);
        default:
            return 0;
    }
    return 0;
}

int get_dev_deta_len(dev_msg1_t *d)
{
    switch (Swap16(d->head.type))
    {
    case E_DEV_SMART_HOUSE:
        return get_smart_house_data_len(Swap16(d->smart_house.head.cmd_type));
    // case E_DEV_SMART_MEDICAL:
    //     return get_smart_medical_res_msg_len(Swap16(d.smart_medical.head.cmd_type));    
    default:
        return 0;
    }
    return 0;
}

// 获取小屋接收服务器消息长度
int get_smart_house_req_msg1_len(smart_house_cmd_type_e type)
{
    switch(type)
    {
        case E_SMART_HOUSE_CMD_RESET:
        case E_SMART_HOUSE_CMD_SEARCH_DEV:
        case E_SMART_HOUSE_CMD_REPORT_STATE:
            return sizeof(msg1_smart_house_head_t);
        case E_SMART_HOUSE_CMD_OPEN_DOOR:
        case E_SMART_HOUSE_CMD_OPEN_CAR_DOOR:
        case E_SMART_HOUSE_CMD_OPEN_WINDOWS:
        case E_SMART_HOUSE_CMD_OPEN_LAMP:
            return sizeof(msg1_smart_house_on_off_t);
        case E_SMART_HOUSE_CMD_SET_SMOKE_ALRAM_VAL:
        case E_SMART_HOUSE_CMD_SET_BRIGHTNESS_VAL:
            return sizeof(msg1_smart_house_smoke_alarm_val_t);
        default:
            return 0;
    }
    return 0;
}

// 获取小屋响应服务器消息长度
int get_smart_house_res_msg1_len(smart_house_cmd_type_e type)
{
    switch(type)
    {
        case E_SMART_HOUSE_CMD_MAX:
        case E_SMART_HOUSE_CMD_RESET:
        case E_SMART_HOUSE_CMD_SEARCH_DEV:
        case E_SMART_HOUSE_CMD_SET_SMOKE_ALRAM_VAL:
        case E_SMART_HOUSE_CMD_SET_BRIGHTNESS_VAL:
            return sizeof(msg1_smart_house_head_t);
        case E_SMART_HOUSE_CMD_REPORT_STATE:
            return sizeof(msg1_smart_house_report_state_t);
        case E_SMART_HOUSE_CMD_OPEN_DOOR:
        case E_SMART_HOUSE_CMD_OPEN_CAR_DOOR:
        case E_SMART_HOUSE_CMD_OPEN_WINDOWS:
        case E_SMART_HOUSE_CMD_OPEN_LAMP:
            return sizeof(msg1_smart_house_on_off_t);
        default:
            return 0;
    }
    return 0;
}

int get_dev_req_msg1_len(dev_msg1_t d)
{
    switch (Swap16(d.head.type))
    {
    case E_DEV_SMART_HOUSE:
        return get_smart_house_req_msg1_len(Swap16(d.smart_house.head.cmd_type));
    // case E_DEV_SMART_MEDICAL:
    //     return get_smart_medical_req_msg_len(Swap16(d.smart_medical.head.cmd_type));    
    default:
        return 0;
    }
    return 0;
}

int get_dev_res_msg1_len(dev_msg1_t d)
{
    switch (Swap16(d.head.type))
    {
    case E_DEV_SMART_HOUSE:
        return get_smart_house_res_msg1_len(Swap16(d.smart_house.head.cmd_type));
    // case E_DEV_SMART_MEDICAL:
    //     return get_smart_medical_res_msg_len(Swap16(d.smart_medical.head.cmd_type));    
    default:
        return 0;
    }
    return 0;
}


extern uint8_t LzEfuseRead(uint32_t offset, uint32_t len, uint8_t *val);

//获取efuse
void get_rk2206_efuse(uint8_t *devnum)
{
    int r = LzEfuseRead(14, sizeof(uint8_devnum), devnum);
    printf("\nget efuse:");
    for(int i = 0; i < r; ++i)
    {
        if(i%16 == 0) printf("\n");
        printf("[%02x]", devnum[i]);
    }
    printf("\n");
}


//获取任务状态字符串
char* get_task_status_string(uint16_t s)
{
    switch(s)
    {
        case OS_TASK_STATUS_UNUSED:
            return "unused";
        case OS_TASK_STATUS_SUSPEND:
            return "suspend";
        case OS_TASK_STATUS_READY:
            return "ready";
        case OS_TASK_STATUS_PEND:
            return "pend";
        case OS_TASK_STATUS_RUNNING:
            return "running";
        case OS_TASK_STATUS_DELAY:
            return "delay";
        case OS_TASK_STATUS_TIMEOUT:
            return "timeout";
        case OS_TASK_STATUS_PEND_TIME:
            return "pend time";
        case OS_TASK_STATUS_EXIT:
            return "exit";
        case OS_TASK_FALG_FREEZE:
            return "freeze";
        case OS_TASK_FLAG_JOINABLE:
            return "joinable";
        default:
            return "unknown";
    }
}

//显示任务信息
uint32_t task_show(int id)
{
    uint32_t i, cnt = 0;
    TSK_INFO_S* ptask_info = NULL;

    ptask_info = (TSK_INFO_S*)calloc(1, sizeof(TSK_INFO_S));
    if (ptask_info == NULL)  return -1;    

    printf("task_info:--------------------------------------------------\r\n");
    if(id < SHOW_ALL_TASK) 
    {
        uint32_t ret = LOS_TaskInfoGet(id, ptask_info);
        if (ret == LOS_OK) {
            ++cnt;
            printf("id=%2d,status=0x%04x %10s,pri=%2hd,size=0x%05x,cur_size=0x%05x,peak_size=0x%05x,stack_top:0x%08x,stack bottom:0x%08x,sp:0x%08x,bOvf:%u,%s\r\n",
                         ptask_info->uwTaskID, ptask_info->usTaskStatus, OsConvertTskStatus(ptask_info->usTaskStatus), 
                         ptask_info->usTaskPrio, ptask_info->uwStackSize, ptask_info->uwCurrUsed, ptask_info->uwPeakUsed, 
                         ptask_info->uwTopOfStack, ptask_info->uwBottomOfStack, ptask_info->uwSP, ptask_info->bOvf,ptask_info->acName);
        }
        else
        {
            printf("task_id err\n");
        }
    }
    else
    {
        for (i = 0; i < g_taskMaxNum; i++) 
        {
            memset_s(ptask_info, sizeof(TSK_INFO_S), 0, sizeof(TSK_INFO_S));
            uint32_t ret = LOS_TaskInfoGet(i, ptask_info);
            if (ret == LOS_OK) {
                ++cnt;
            printf("id=%2d,status=0x%04x %10s,pri=%2hd,size=0x%05x,cur_size=0x%05x,peak_size=0x%05x,stack_top:0x%08x,stack bottom:0x%08x,sp:0x%08x,bOvf:%u,%s\r\n",
                         ptask_info->uwTaskID, ptask_info->usTaskStatus, OsConvertTskStatus(ptask_info->usTaskStatus), 
                         ptask_info->usTaskPrio, ptask_info->uwStackSize, ptask_info->uwCurrUsed, ptask_info->uwPeakUsed, 
                         ptask_info->uwTopOfStack, ptask_info->uwBottomOfStack, ptask_info->uwSP, ptask_info->bOvf,ptask_info->acName);
            }
        }
    }
    printf("task num:%d\n", cnt);

    free(ptask_info);
    return LOS_OK;
}

//创建任务
int create_task(unsigned int *threadID,  uint32_t size, uint8_t prio, ThreadFunc func, void *args, const char *owner)
{
    int              ret  = 0;
    TSK_INIT_PARAM_S task = {0};

    task.pfnTaskEntry = (TSK_ENTRY_FUNC)func;
    task.uwStackSize  = size;
    task.pcName       = (char*)owner;
    task.uwArg        = (uint32_t)args;
    task.usTaskPrio   = prio>30?30:prio;
    ret = LOS_TaskCreate(threadID, &task);
    if(ret != LOS_OK) LZ_HARDWARE_LOGE("create_task", "ret = 0x%x\n", ret);

    return (ret == LOS_OK) ? LZ_HARDWARE_SUCCESS : LZ_HARDWARE_FAILURE;
}


//获取定时器的计数
uint_tick get_current_tick(void)
{
#if     TICK_BIT_64
    uint32_t  low, high, temp;
    // uint32_t  low1, high1, temp1;
    // int cnt = 0;
    do{
        high = TIMER5->CURRENT_VALUE[1];
        low  = TIMER5->CURRENT_VALUE[0];
        temp = TIMER5->CURRENT_VALUE[1];
        // if(cnt == 0)
        // {
        //     high1 = high;
        //     low1  = low;
        //     temp1 = temp;
        // }
        // cnt++;
    }while(high != temp);

    // if(cnt>1) printf("cnt:%d, h:%u,l:%u,t:%u\ncnt1   h:%u,l:%u,t:%u\n",cnt, high1, low1, temp1, high, low, temp);
    return ((uint_tick)high << 32) | low;
#else
    return TIMER5->CURRENT_VALUE[0];
#endif
}

//计算时间 输入起始定时器计数，返回时间差 单位us  如果函数中1000000改成1000 单位ms
uint_tick get_time_us(uint_tick start_tick)
{
    #define TICK_TIME_US       1000000
    uint_tick end_tick = get_current_tick();
    if (end_tick > start_tick)
    {
        return (end_tick - start_tick) / (PLL_INPUT_OSC_RATE / TICK_TIME_US);
    }

    // printf("%s()"FORMAT_UINT_TICK"-"FORMAT_UINT_TICK":"FORMAT_UINT_TICK"\n",__FUNCTION__, start_tick, end_tick, (TICK_OVER_VALUE - start_tick + end_tick) / (PLL_INPUT_OSC_RATE / TICK_TIME_US));
    return (TICK_OVER_VALUE - start_tick + end_tick) / (PLL_INPUT_OSC_RATE / TICK_TIME_US);
}

WEAK void dump_hex(printf_fn pfn, char *info, uint8_t *buf, int len)
{
	int i,j = 0;
	char str_buff[17];
	if(pfn == NULL) return;
	
	pfn("\n(%s) data len:%d\n", info, len);
	pfn("Address  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | a | b | c | d | e | f |\n");	
	for(i = 0; i < len; i++)
	{		
		if(i%16 == 0)
		{
			str_buff[16] = 0;
			if(i != 0)
			{
				pfn("|%s|\n", str_buff);				
			}
			pfn("0x%06X:",j*16);
			j++;
		}
		if(*(buf+i) >= 0x20 && *(buf+i) <= 0x7e)
		{
			str_buff[i%16] = *(buf+i);			
		}
		else
		{
			str_buff[i%16] = 0x2e;
		}
		pfn(" %02x ", *(buf+i));
	}
	
	if(i != 0)
	{
		if( i%16 != 0)
		{		
			for(j = 0; j < 16-(i%16); j++)
			{
				str_buff[i%16+j] = 0x20;
				pfn("    ");
			}
			str_buff[16] = 0;
			pfn("|%s|", str_buff);				
		}
		else
		{
			pfn("|%s|", str_buff);							
		}
		
	}
	
	pfn("\n\n");
}

