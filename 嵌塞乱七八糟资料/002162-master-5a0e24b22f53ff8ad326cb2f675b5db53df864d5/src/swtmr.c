
#include "swtmr.h"
#include "lz_hardware.h"

//获取id
int swtmr_getid(void *tmr, char *name, uint32_t *id, uint8_t *index)
{
    swtmr_t  *t = (swtmr_t*)tmr;
    if(name == NULL) 
    {
        return -1;
    }
    
    for(int i = 0; i < t->cnt; ++i)
    {
        if(t->time[i].name == NULL)continue;
        if(strcmp(name, t->time[i].name) == 0)
        {
            *id = t->time[i].id;
            *index = i;
            return 0;
        }
    }
    return -2;
}

// 获取启动状态
// 返回1 已启动， 返回0 未启动， 返回-1 不存在
int swtmr_get_start_state(void *tmr, char *name)
{
    int      ret = 0;
    uint32_t id  = 0;
    uint8_t  index = 0;
    swtmr_t *t = (swtmr_t*)tmr;

    ret = t->getid_fn(tmr, name, &id, &index);
    if (ret != LOS_OK)
    {
        printf("swtimer (%s) getid fail ret:%d,%d\n", name, ret, t->cnt);
        return -1;
    }

    return t->time[index].start_flag;
}

//启动定时器
void swtmr_start(void *tmr, char *name)
{
    int      ret = 0;
    uint32_t id  = 0;
    uint8_t  index = 0;
    swtmr_t *t = (swtmr_t*)tmr;

    ret = t->getid_fn(tmr, name, &id, &index);
    if (ret != LOS_OK)
    {
        printf("start swtimer (%s) getid fail ret:%d,%d\n", name, ret, t->cnt);
        return;
    }

    // printf("\n@#swtimer (%s) start (%d)\n", name, id);
    // if(t->time[index].start_flag) return;

    ret = LOS_SwtmrStart(id);
    if (ret != LOS_OK)
    {
        printf("start swtimer (%s) fail ret:%d\n", name, ret);
    }
    t->time[index].start_flag = 1;
}

//停止定时器
void swtmr_stop(void *tmr, char *name)
{
    int      ret   = 0;
    uint8_t  index = 0;
    uint32_t id    = 0;
    swtmr_t *t = (swtmr_t*)tmr;


    ret = t->getid_fn(tmr, name, &id, &index);
    if (ret != LOS_OK)
    {
        printf("stop swtimer (%s) getid fail ret:%d\n", name, ret);
        return;
    }

    if(t->time[index].start_flag == 0) return;

    // printf("\n@#swtimer (%s) stop (%d)\n", name, id);

    ret = LOS_SwtmrStop(id);
    if (ret != LOS_OK)
    {
        printf("stop swtimer (%s) fail ret:%d\n", name, ret);
    }
    t->time[index].start_flag = 0;
}

/***************************************************************
* 函数名称: swtmr_init
* 说    明: 软件定时器初始化
* 参    数: 
* 返 回 值: 无
***************************************************************/
int swtmr_init(unsigned int period, SWTMR_PROC_FUNC fn, char *name, unsigned int *args)
{
    unsigned int ret;
    swtmr_t  *tmr = (swtmr_t*)args;

    if(name == NULL) return -3;
    if(tmr->cnt >= SWTMR_CNT_MAX) return -2;

    tmr->time[tmr->cnt].name = name;

    // printf("tmr swtimer init %s\n", tmr->time[tmr->cnt].name);
    ret = LOS_SwtmrCreate(period, LOS_SWTMR_MODE_PERIOD, fn, &tmr->time[tmr->cnt].id, (uint32_t)tmr->args);
    if (ret != LOS_OK)
    {
        printf("create timer (%s) fail ret:0x%x\n", name, ret);
        return -1;
    }

    ++tmr->cnt;
    
    return 0;
}
