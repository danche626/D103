#ifndef _SWTMR_H
#define _SWTMR_H

#include "los_swtmr.h"

typedef int  (*pswtmr_getid)(void*, char*, uint32_t*, uint8_t *);
typedef int  (*pswtmr_get_start_state)(void *, char *);
typedef void (*pswtmr_start)(void*, char*);
typedef void (*pswtmr_stop)(void*, char*);
typedef int  (*pswtmr_init)(unsigned int, SWTMR_PROC_FUNC, char*, unsigned int*);

#define   SWTMR_CNT_MAX         10

typedef struct
{
    uint32_t                    id;
    char                        *name;
    uint8_t                     start_flag;            //启动标志1启动
} swtmrid_t;

typedef struct
{
    swtmrid_t                   time[SWTMR_CNT_MAX];
    uint8_t                     cnt;
    void *                      args;

    pswtmr_getid                getid_fn;
    pswtmr_get_start_state      get_start_state_fn;
    pswtmr_start                start_fn;
    pswtmr_stop                 stop_fn;
    pswtmr_init                 init_fn;
} swtmr_t;


int swtmr_getid(void *tk, char *name, uint32_t *id, uint8_t *index);
int swtmr_get_start_state(void *tmr, char *name);
void swtmr_start(void *tmr, char *name);
void swtmr_stop(void *tmr, char *name);
int swtmr_init(unsigned int period, SWTMR_PROC_FUNC fn, char *name, unsigned int *args);





















#endif